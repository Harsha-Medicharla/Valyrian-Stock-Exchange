#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "db/DBWriter.h"
#include "db/SymbolCache.h"
#include "ems/core/EMSCore.h"
#include "ems/pipeline/BalanceCache.h"
#include "ems/pipeline/ValidationPipeline.h"
#include "ems/pipeline/MarketState.h"
#include "ems/pipeline/RateLimiter.h"
#include "market_data/MarketDataPublisher.h"
#include "shared/types/Events.h"
#include "shared/queues/EventSPSC.h"
#include "trade_server/network/ConnTable.h"
#include "trade_server/network/WebSocketServer.h"
#include "trade_server/resp/RespThread.h"
#include "api_server/db/RedisPool.h"
#include <drogon/drogon.h>

namespace
{

    struct MockTradeRow
    {
        uint32_t symbol_id;
        uint64_t buyer_order_id;
        uint64_t seller_order_id;
        int64_t price;
        int64_t qty;
    };

    class MockPGWriter final : public IDBWriterBackend
    {
    public:
        std::mutex mutex;
        std::vector<DBEvent> events;
        std::vector<MockTradeRow> trades;
        std::unordered_map<uint64_t, OrderState> orders;

        void writeBatch(const std::vector<DBEvent> &batch) override
        {
            std::lock_guard lock(mutex);
            for (const DBEvent &ev : batch)
            {
                events.push_back(ev);
                if (ev.type == DBEventType::ORDER_ACCEPTED)
                    orders[ev.order_id] = OrderState::NEW;
                else if (ev.type == DBEventType::ORDER_FILLED)
                {
                    orders[ev.order_id] = ev.state;
                    if (ev.side == Side::BUY)
                        trades.push_back(MockTradeRow{ev.symbol_id, ev.order_id,
                                                      ev.peer_order_id, ev.fill_price,
                                                      ev.fill_qty});
                }
                else if (ev.type == DBEventType::ORDER_CANCELLED)
                    orders[ev.order_id] = OrderState::CANCELLED;
            }
        }

        void reset()
        {
            std::lock_guard lock(mutex);
            events.clear();
            trades.clear();
            orders.clear();
        }
    };

    RawOrder makeOrder(uint64_t seq, uint64_t oid, uint32_t uid, uint32_t sym,
                       Side side, OrderType type, int64_t px, uint32_t qty)
    {
        RawOrder o{};
        o.sequence = seq;
        o.order_id = oid;
        o.user_id = uid;
        o.symbol_id = sym;
        o.price = px;
        o.qty = qty;
        o.side = static_cast<uint8_t>(side);
        o.type = static_cast<uint8_t>(type);
        o.timestamp = seq;
        o.cancel_flag = 0;
        o.modify_flag = 0;
        return o;
    }

    RawOrder makeCancel(uint64_t seq, uint64_t target_oid)
    {
        RawOrder o{};
        o.sequence = seq;
        o.order_id = target_oid;
        o.cancel_flag = 1;
        o.timestamp = seq;
        return o;
    }

    inline void clearWalFiles()
    {
        std::remove("engine_0.wal");
        std::remove("engine_0.trades");
        std::remove("/app/wal/engine_0.wal");
        std::remove("/app/wal/engine_0.trades");
    }

}

class TradingSystemTest : public ::testing::Test
{
protected:
    SymbolCache symbolCache;

    void SetUp() override
    {
        clearWalFiles();
        symbolCache.loadFromList({SymbolInfo{0, "AAPL", 1, 1, true}});
    }

    void TearDown() override { clearWalFiles(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1 — Full E2E Pipeline: Ingress → Match → DB → Balance settlement
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, PipelineAcceptsMatchesAndSettles)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 100, 0);
    ems.balanceCache().setBalance(2, 10000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    MarketDataPublisher marketData(ems.tradeQueues(), ems.bookUpdateQueues(),
                                   1, 0);
    WebSocketServer ws(ems, 0, false);
    RespThread resp(ems, ws);

    ems.start();
    dbWriter.start();
    marketData.start();
    resp.start();

    ASSERT_TRUE(ems.spscQueue(0).enqueue(
        makeOrder(1, 101, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10)));
    ASSERT_TRUE(ems.spscQueue(0).enqueue(
        makeOrder(2, 102, 2, 0, Side::BUY, OrderType::LIMIT, 100, 10)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    resp.stop();
    resp.join();
    marketData.stop();
    marketData.join();
    dbWriter.stop();
    dbWriter.join();
    ems.stop();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    ASSERT_EQ(mockWriter.trades.size(), 1u);
    EXPECT_EQ(mockWriter.trades[0].price, 100);
    EXPECT_EQ(mockWriter.trades[0].qty, 10);
    EXPECT_EQ(mockWriter.orders[101], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[102], OrderState::FILLED);
    EXPECT_EQ(ems.balanceCache().availableBalance(2), 9000);   // 10000 - 1000
    EXPECT_EQ(ems.balanceCache().availableBalance(1), 1000);   // 0     + 1000
    EXPECT_EQ(ems.balanceCache().availableHoldings(2, 0), 10); // 0     + 10
    EXPECT_EQ(ems.balanceCache().availableHoldings(1, 0), 90); // 100   - 10
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2 — Determinism: price/time priority sweep across multiple levels
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, DeterministicBookSweep)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 1000, 0);
    ems.balanceCache().setBalance(2, 100000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    ems.spscQueue(0).enqueue(makeOrder(1, 1, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
    ems.spscQueue(0).enqueue(makeOrder(2, 2, 1, 0, Side::SELL, OrderType::LIMIT, 101, 50));
    ems.spscQueue(0).enqueue(makeOrder(3, 3, 1, 0, Side::SELL, OrderType::LIMIT, 102, 50));

    ems.spscQueue(0).enqueue(makeOrder(4, 4, 2, 0, Side::BUY, OrderType::MARKET, 0, 120));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    ASSERT_EQ(mockWriter.trades.size(), 3u);
    EXPECT_EQ(mockWriter.trades[0].seller_order_id, 1u);
    EXPECT_EQ(mockWriter.trades[0].qty, 50);
    EXPECT_EQ(mockWriter.trades[0].price, 100);
    EXPECT_EQ(mockWriter.trades[1].seller_order_id, 2u);
    EXPECT_EQ(mockWriter.trades[1].qty, 50);
    EXPECT_EQ(mockWriter.trades[1].price, 101);
    EXPECT_EQ(mockWriter.trades[2].seller_order_id, 3u);
    EXPECT_EQ(mockWriter.trades[2].qty, 20);
    EXPECT_EQ(mockWriter.trades[2].price, 102);
    EXPECT_EQ(mockWriter.orders[1], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[2], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[3], OrderState::PARTIALLY_FILLED);
    EXPECT_EQ(mockWriter.orders[4], OrderState::FILLED);
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3 — Pre-trade risk: order rejected when buyer has insufficient funds
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, RejectsOrdersWithoutSufficientFunds)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setBalance(1, 50, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    ASSERT_TRUE(ems.spscQueue(0).enqueue(
        makeOrder(1, 101, 1, 0, Side::BUY, OrderType::LIMIT, 100, 1)));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    EXPECT_TRUE(mockWriter.orders.empty());
    EXPECT_TRUE(mockWriter.trades.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 4 — Self-trade prevention: resting order cancelled, no trade emitted
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, SelfTradePreventionCancelsRestingOrder)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 100, 0);
    ems.balanceCache().setBalance(1, 10000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    ems.spscQueue(0).enqueue(makeOrder(1, 201, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10));
    ems.spscQueue(0).enqueue(makeOrder(2, 202, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    EXPECT_TRUE(mockWriter.trades.empty());
    EXPECT_EQ(mockWriter.orders[201], OrderState::CANCELLED);
    EXPECT_EQ(mockWriter.orders[202], OrderState::NEW);
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 5 — Cancel releases blocked funds
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, CancelOrderReleasesBlockedFunds)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setBalance(1, 5000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    ems.spscQueue(0).enqueue(makeOrder(1, 301, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));
    // Immediately cancel it.
    ems.spscQueue(0).enqueue(makeCancel(2, 301));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    EXPECT_EQ(mockWriter.orders[301], OrderState::CANCELLED);
    EXPECT_EQ(ems.balanceCache().availableBalance(1), 5000);
    EXPECT_EQ(ems.balanceCache().blockedBalance(1), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 6 — Price/time priority: two sellers at same price, one at worse price
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, PriceTimePriorityBookSweep)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 1000, 0);
    ems.balanceCache().setHoldings(2, 0, 1000, 0);
    ems.balanceCache().setBalance(3, 100000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    ems.spscQueue(0).enqueue(makeOrder(1, 401, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
    ems.spscQueue(0).enqueue(makeOrder(2, 402, 2, 0, Side::SELL, OrderType::LIMIT, 100, 50));
    ems.spscQueue(0).enqueue(makeOrder(3, 403, 1, 0, Side::SELL, OrderType::LIMIT, 101, 50));
    ems.spscQueue(0).enqueue(makeOrder(4, 404, 3, 0, Side::BUY, OrderType::MARKET, 0, 120));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    ASSERT_EQ(mockWriter.trades.size(), 3u);
    EXPECT_EQ(mockWriter.trades[0].seller_order_id, 401u);
    EXPECT_EQ(mockWriter.trades[0].qty, 50);
    EXPECT_EQ(mockWriter.trades[1].seller_order_id, 402u);
    EXPECT_EQ(mockWriter.trades[1].qty, 50);
    EXPECT_EQ(mockWriter.trades[2].seller_order_id, 403u);
    EXPECT_EQ(mockWriter.trades[2].qty, 20);
    EXPECT_EQ(mockWriter.trades[2].price, 101);
    EXPECT_EQ(mockWriter.orders[401], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[402], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[403], OrderState::PARTIALLY_FILLED);
    EXPECT_EQ(mockWriter.orders[404], OrderState::FILLED);
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 7 — WAL crash recovery: engine state survives restart
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, CrashAndRecoveryDeterminism)
{
    {
        EMSCore ems(1, 1, symbolCache);
        ems.balanceCache().setHoldings(1, 0, 100, 0);
        ems.start();
        ems.spscQueue(0).enqueue(makeOrder(1, 5001, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ems.stop();
        ems.join();
    }

    MockPGWriter mockWriter;
    EMSCore recoveredEms(1, 1, symbolCache);
    recoveredEms.balanceCache().setHoldings(1, 0, 50, 50);
    recoveredEms.balanceCache().setBalance(2, 5000, 0);

    DBWriter dbWriter(recoveredEms.ingressDbQueues(), recoveredEms.engineDbQueues(),
                      1, recoveredEms.balanceCache(), mockWriter);
    recoveredEms.start();
    dbWriter.start();

    recoveredEms.spscQueue(0).enqueue(makeOrder(1, 5002, 2, 0, Side::BUY, OrderType::LIMIT, 100, 50));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    recoveredEms.stop();
    dbWriter.stop();
    recoveredEms.join();
    dbWriter.join();

    std::lock_guard lock(mockWriter.mutex);
    ASSERT_EQ(mockWriter.trades.size(), 1u);
    EXPECT_EQ(mockWriter.trades[0].qty, 50);
    EXPECT_EQ(mockWriter.trades[0].price, 100);
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 8 — High-throughput: 10,000 orders, zero dropped messages
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TradingSystemTest, HighThroughputDeterminismNoDataLoss)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 1000000, 0);
    ems.balanceCache().setBalance(2, 100000000, 0);

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1,
                      ems.balanceCache(), mockWriter);
    ems.start();
    dbWriter.start();

    const int ORDER_COUNT = 10000;
    for (int i = 1; i <= ORDER_COUNT; ++i)
    {
        if (i % 2 != 0)
            while (!ems.spscQueue(0).enqueue(
                makeOrder(i, i, 1, 0, Side::SELL, OrderType::LIMIT, 100, 1)))
                ;
        else
            while (!ems.spscQueue(0).enqueue(
                makeOrder(i, i, 2, 0, Side::BUY, OrderType::LIMIT, 100, 1)))
                ;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    dbWriter.stop();
    ems.stop();
    dbWriter.join();
    ems.join();

    std::lock_guard lock(mockWriter.mutex);
    EXPECT_EQ(mockWriter.trades.size(), 5000u);
    EXPECT_EQ(mockWriter.orders.size(), 10000u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Module wiring tests (active, no pipeline spin-up required)
// ─────────────────────────────────────────────────────────────────────────────

TEST(EMSModuleTest, BalanceLocking)
{
    BalanceCache bc(1);
    bc.setBalance(10, 5000, 0);
    EXPECT_TRUE(bc.tryBlockFunds(10, 2000));
    EXPECT_EQ(bc.availableBalance(10), 3000);
    EXPECT_EQ(bc.blockedBalance(10), 2000);
    bc.unblockFunds(10, 2000);
    EXPECT_EQ(bc.availableBalance(10), 5000);
}

TEST(EMSModuleTest, PipelineValidation)
{
    RateLimiter rl(100000);
    MarketState ms(1);
    BalanceCache bc(1);
    SymbolCache sc;
    sc.loadFromList({{0, "AAPL", 1, 1, true}});
    ValidationPipeline pipeline(rl, ms, bc, sc);
    RejectReason reason;

    RawOrder marketOrder{};
    marketOrder.symbol_id = 0;
    marketOrder.type = static_cast<uint8_t>(OrderType::MARKET);
    marketOrder.price = 0;
    marketOrder.qty = 10;
    bc.setBalance(1, 10000, 0);

    EXPECT_EQ(pipeline.process(&marketOrder, reason), Decision::ACCEPT);
}

TEST(TradeServerModuleTest, ConnTableLogic)
{
    ConnTable table;
    uint32_t connIdx = table.assign(100);
    EXPECT_NE(connIdx, 0u);
    EXPECT_EQ(table.findByUser(100), connIdx);
    table.release(connIdx);
    EXPECT_EQ(table.findByUser(100), 0u);
}

TEST(DBWriterModuleTest, QueueHandling)
{
    BalanceCache bc(1);
    class NullBackend final : public IDBWriterBackend
    {
    public:
        void writeBatch(const std::vector<DBEvent> &) override {}
    };
    NullBackend backend;
    std::vector<EventSPSC<DBEvent>> inQ;
    inQ.emplace_back(128);
    std::vector<EventSPSC<DBEvent>> enQ;
    enQ.emplace_back(128);
    DBWriter writer(inQ, enQ, 1, bc, backend);
    SUCCEED();
}

TEST(APIServerModuleTest, RedisPoolSingleton)
{
    RedisPool &pool = RedisPool::instance();
    pool.init("127.0.0.1", 6379);
    SUCCEED();
}

TEST(APIServerModuleTest, DrogonAppCheck)
{
    auto &app = drogon::app();
    EXPECT_NE(&app, nullptr);
}
