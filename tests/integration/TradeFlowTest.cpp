// #include <atomic>
// #include <chrono>
// #include <cstdint>
// #include <mutex>
// #include <thread>
// #include <unordered_map>
// #include <vector>

// #include <gtest/gtest.h>

// #include "db/DBWriter.h"
// #include "db/SymbolCache.h"
// #include "ems/core/EMSCore.h"
// #include "market_data/MarketDataPublisher.h"
// #include "trade_server/network/WebSocketServer.h"
// #include "trade_server/resp/RespThread.h"
// #include "MatchingEngine/wal/WAL.h"

// namespace
// {
// struct MockTradeRow
// {
//     uint32_t symbol_id;
//     uint64_t buyer_order_id;
//     uint64_t seller_order_id;
//     int64_t price;
//     int64_t qty;
// };

// // A thread-safe mock database to intercept DBWriter's output
// class MockPGWriter final : public IDBWriterBackend
// {
// public:
//     std::mutex mutex;
//     std::vector<DBEvent> events;
//     std::vector<MockTradeRow> trades;
//     std::unordered_map<uint64_t, OrderState> orders;

//     void writeBatch(const std::vector<DBEvent> &batch) override
//     {
//         std::lock_guard lock(mutex);
//         for (const DBEvent &event : batch)
//         {
//             events.push_back(event);
//             if (event.type == DBEventType::ORDER_ACCEPTED)
//             {
//                 orders[event.order_id] = OrderState::NEW;
//             }
//             else if (event.type == DBEventType::ORDER_FILLED)
//             {
//                 orders[event.order_id] = event.state;
//                 // Only log trades from the buyer side to avoid double-counting
//                 if (event.side == Side::BUY)
//                 {
//                     trades.push_back(MockTradeRow{
//                         event.symbol_id,
//                         event.order_id,
//                         event.peer_order_id,
//                         event.fill_price,
//                         event.fill_qty});
//                 }
//             }
//             else if (event.type == DBEventType::ORDER_CANCELLED)
//             {
//                 orders[event.order_id] = OrderState::CANCELLED;
//             }
//         }
//     }

//     void reset() {
//         std::lock_guard lock(mutex);
//         events.clear();
//         trades.clear();
//         orders.clear();
//     }
// };

// // Helper function to create raw orders
// RawOrder makeOrder(uint64_t seq, uint64_t oid, uint32_t uid, uint32_t sym, Side side, OrderType type, int64_t px, uint32_t qty) {
//     RawOrder o{};
//     o.sequence = seq;
//     o.order_id = oid;
//     o.user_id = uid;
//     o.symbol_id = sym;
//     o.price = px;
//     o.qty = qty;
//     o.side = static_cast<uint8_t>(side);
//     o.type = static_cast<uint8_t>(type);
//     o.timestamp = seq; // Using sequence as deterministic timestamp
//     return o;
// }
// } // namespace

// class TradingSystemTest : public ::testing::Test {
// protected:
//     SymbolCache symbolCache;

//     void SetUp() override {
//         // Clear WAL files before each test for clean state
//         std::remove("engine_0.wal");
//         std::remove("engine_0.trades");
        
//         symbolCache.loadFromList({
//             SymbolInfo{0, "AAPL", 1, 1, true},
//         });
//     }

//     void TearDown() override {
//         std::remove("engine_0.wal");
//         std::remove("engine_0.trades");
//     }
// };

// // -----------------------------------------------------------------------------
// // TEST 1: Full E2E Pipeline Integrity
// // -----------------------------------------------------------------------------
// TEST_F(TradingSystemTest, PipelineAcceptsMatchesAndSettles)
// {
//     EMSCore ems(1, 1, symbolCache);
//     // Give User 1 (Seller) 100 shares of AAPL
//     ems.balanceCache().setHoldings(1, 0, 100, 0);
//     // Give User 2 (Buyer) $10,000
//     ems.balanceCache().setBalance(2, 10000, 0);

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     MarketDataPublisher marketData(ems.tradeQueues(), 1, 0);
//     WebSocketServer ws(ems, 0);
//     RespThread resp(ems, ws);

//     ems.start(); dbWriter.start(); marketData.start(); resp.start();

//     // 1. Seller places Limit order: Sell 10 @ $100
//     ASSERT_TRUE(ems.spscQueue(0).enqueue(
//         makeOrder(1, 101, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10)
//     ));

//     // 2. Buyer places Limit order: Buy 10 @ $100
//     ASSERT_TRUE(ems.spscQueue(0).enqueue(
//         makeOrder(2, 102, 2, 0, Side::BUY, OrderType::LIMIT, 100, 10)
//     ));

//     // Wait for settlement (drain queues)
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     resp.stop(); marketData.stop(); dbWriter.stop(); ems.stop();
//     resp.join(); marketData.join(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     // Validate DB Events
//     ASSERT_EQ(mockWriter.trades.size(), 1u);
//     EXPECT_EQ(mockWriter.trades[0].price, 100);
//     EXPECT_EQ(mockWriter.trades[0].qty, 10);
    
//     // Validate Order States
//     EXPECT_EQ(mockWriter.orders[101], OrderState::FILLED);
//     EXPECT_EQ(mockWriter.orders[102], OrderState::FILLED);

//     // Validate Lock-Free Balance Settlement
//     // Buyer spent $1,000. Seller gained $1,000.
//     EXPECT_EQ(ems.balanceCache().availableBalance(2), 9000); // 10000 - 1000
//     EXPECT_EQ(ems.balanceCache().availableBalance(1), 1000); // 0 + 1000
    
//     // Buyer gained 10 shares. Seller lost 10 shares.
//     EXPECT_EQ(ems.balanceCache().availableHoldings(2, 0), 10); // 0 + 10
//     EXPECT_EQ(ems.balanceCache().availableHoldings(1, 0), 90); // 100 - 10
// }

// // -----------------------------------------------------------------------------
// // TEST 2: Determinism Under Load & Partial Sweeps
// // -----------------------------------------------------------------------------
// TEST_F(TradingSystemTest, DeterministicBookSweep)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setHoldings(1, 0, 1000, 0); // Seller has 1000 shares
//     ems.balanceCache().setBalance(2, 100000, 0);   // Buyer has enough cash

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     // Seller builds a ladder of liquidity
//     ems.spscQueue(0).enqueue(makeOrder(1, 1, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
//     ems.spscQueue(0).enqueue(makeOrder(2, 2, 1, 0, Side::SELL, OrderType::LIMIT, 101, 50));
//     ems.spscQueue(0).enqueue(makeOrder(3, 3, 1, 0, Side::SELL, OrderType::LIMIT, 102, 50));

//     // Buyer places a sweeping market order for 120 shares
//     ems.spscQueue(0).enqueue(makeOrder(4, 4, 2, 0, Side::BUY, OrderType::MARKET, 0, 120));

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     // Should result in exactly 3 trades
//     ASSERT_EQ(mockWriter.trades.size(), 3u);
    
//     // Validating Price/Time Priority and Deterministic execution
//     EXPECT_EQ(mockWriter.trades[0].seller_order_id, 1);
//     EXPECT_EQ(mockWriter.trades[0].qty, 50);
//     EXPECT_EQ(mockWriter.trades[0].price, 100);

//     EXPECT_EQ(mockWriter.trades[1].seller_order_id, 2);
//     EXPECT_EQ(mockWriter.trades[1].qty, 50);
//     EXPECT_EQ(mockWriter.trades[1].price, 101);

//     EXPECT_EQ(mockWriter.trades[2].seller_order_id, 3);
//     EXPECT_EQ(mockWriter.trades[2].qty, 20); // Only 20 needed to finish the 120
//     EXPECT_EQ(mockWriter.trades[2].price, 102);

//     // Validate Final State
//     EXPECT_EQ(mockWriter.orders[1], OrderState::FILLED);
//     EXPECT_EQ(mockWriter.orders[2], OrderState::FILLED);
//     EXPECT_EQ(mockWriter.orders[3], OrderState::PARTIALLY_FILLED);
//     EXPECT_EQ(mockWriter.orders[4], OrderState::FILLED);
// }

// // -----------------------------------------------------------------------------
// // TEST 3: Pre-Trade Risk & Rejection
// // -----------------------------------------------------------------------------
// TEST_F(TradingSystemTest, RejectsOrdersWithoutSufficientFunds)
// {
//     EMSCore ems(1, 1, symbolCache);
//     // User 1 has only $50
//     ems.balanceCache().setBalance(1, 50, 0);

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     // Try to buy $100 worth of stock
//     ASSERT_TRUE(ems.spscQueue(0).enqueue(
//         makeOrder(1, 101, 1, 0, Side::BUY, OrderType::LIMIT, 100, 1)
//     ));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
//     // Should never hit the database as an accepted order
//     EXPECT_TRUE(mockWriter.orders.empty());
//     EXPECT_TRUE(mockWriter.trades.empty());
// }


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
#include "market_data/MarketDataPublisher.h"
#include "trade_server/network/WebSocketServer.h"
#include "trade_server/resp/RespThread.h"
#include "MatchingEngine/wal/WAL.h"

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

// Thread-safe mock database to intercept and verify DBWriter's output
class MockPGWriter final : public IDBWriterBackend
{
public:
    std::mutex mutex;
    std::vector<DBEvent> events;
    std::vector<MockTradeRow> trades;
    std::unordered_map<uint64_t, OrderState> orders;
    std::atomic<int> total_events_processed{0};

    void writeBatch(const std::vector<DBEvent> &batch) override
    {
        std::lock_guard lock(mutex);
        for (const DBEvent &event : batch)
        {
            events.push_back(event);
            total_events_processed++;
            
            if (event.type == DBEventType::ORDER_ACCEPTED) {
                orders[event.order_id] = OrderState::NEW;
            }
            else if (event.type == DBEventType::ORDER_FILLED) {
                orders[event.order_id] = event.state; // FILLED or PARTIALLY_FILLED
                // Log trades from the buyer side to avoid double-counting
                if (event.side == Side::BUY) {
                    trades.push_back(MockTradeRow{
                        event.symbol_id, event.order_id, event.peer_order_id,
                        event.fill_price, event.fill_qty});
                }
            }
            else if (event.type == DBEventType::ORDER_CANCELLED) {
                orders[event.order_id] = OrderState::CANCELLED;
            }
        }
    }
};

// Helper to create limit/market orders
RawOrder makeOrder(uint64_t seq, uint64_t oid, uint32_t uid, uint32_t sym, Side side, OrderType type, int64_t px, uint32_t qty) {
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
    return o;
}

// Helper to create a cancellation request
RawOrder makeCancel(uint64_t seq, uint64_t oid) {
    RawOrder o{};
    o.sequence = seq;
    o.order_id = oid;
    o.cancel_flag = 1;
    o.timestamp = seq;
    return o;
}
} // namespace

class ExhaustiveTradingTest : public ::testing::Test {
protected:
    SymbolCache symbolCache;

    void SetUp() override {
        std::remove("engine_0.wal");
        std::remove("engine_0.trades");
        symbolCache.loadFromList({ SymbolInfo{0, "AAPL", 1, 1, true} });
    }

    void TearDown() override {
        std::remove("engine_0.wal");
        std::remove("engine_0.trades");
    }
};


// ============================================================================
// TEST 1: The Vanilla Flow (Clean Match & Settle)
// ============================================================================
TEST_F(ExhaustiveTradingTest, StandardMatchAndBalanceSettle)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 100, 0); // Seller: 100 shares
    ems.balanceCache().setBalance(2, 10000, 0);   // Buyer: $10,000

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
    ems.start(); dbWriter.start();

    ems.spscQueue(0).enqueue(makeOrder(1, 101, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10));
    ems.spscQueue(0).enqueue(makeOrder(2, 102, 2, 0, Side::BUY, OrderType::LIMIT, 100, 10));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

    std::lock_guard lock(mockWriter.mutex);
    ASSERT_EQ(mockWriter.trades.size(), 1u);
    EXPECT_EQ(ems.balanceCache().availableBalance(2), 9000); 
    EXPECT_EQ(ems.balanceCache().availableHoldings(2, 0), 10); 
}

// ============================================================================
// TEST 2: Self-Trade Prevention (Wash Trading Shield)
// ============================================================================
TEST_F(ExhaustiveTradingTest, SelfTradePreventionCancelsRestingOrder)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 100, 0); 
    ems.balanceCache().setBalance(1, 10000, 0);   

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
    ems.start(); dbWriter.start();

    // User 1 places a SELL order
    ems.spscQueue(0).enqueue(makeOrder(1, 201, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10));
    // User 1 tries to BUY their own order
    ems.spscQueue(0).enqueue(makeOrder(2, 202, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

    std::lock_guard lock(mockWriter.mutex);
    
    // Engine MUST NOT generate a trade
    EXPECT_TRUE(mockWriter.trades.empty());
    
    // Engine MUST cancel the resting order (201) to prevent the self-trade
    EXPECT_EQ(mockWriter.orders[201], OrderState::CANCELLED);
    
    // The incoming order (202) should rest on the book as NEW since it had no valid matches
    EXPECT_EQ(mockWriter.orders[202], OrderState::NEW);
}

// ============================================================================
// TEST 3: Asynchronous Cancellation & Fund Un-blocking
// ============================================================================
TEST_F(ExhaustiveTradingTest, CancelOrderReleasesBlockedFunds)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setBalance(1, 5000, 0); // Start with $5,000

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
    ems.start(); dbWriter.start();

    // 1. Buy $1,000 worth of stock. Should block $1,000.
    ems.spscQueue(0).enqueue(makeOrder(1, 301, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));
    
    // 2. Cancel the order immediately.
    ems.spscQueue(0).enqueue(makeCancel(2, 301));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

    std::lock_guard lock(mockWriter.mutex);
    
    EXPECT_EQ(mockWriter.orders[301], OrderState::CANCELLED);

    // Verify Funds were fully released back to Available
    EXPECT_EQ(ems.balanceCache().availableBalance(1), 5000);
    EXPECT_EQ(ems.balanceCache().blockedBalance(1), 0);
}

// ============================================================================
// TEST 4: Partial Fills & Price/Time Priority Sweeping
// ============================================================================
TEST_F(ExhaustiveTradingTest, PriceTimePriorityBookSweep)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 1000, 0); 
    ems.balanceCache().setHoldings(2, 0, 1000, 0); 
    ems.balanceCache().setBalance(3, 100000, 0);   

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
    ems.start(); dbWriter.start();

    // Seller 1 places order at $100 (Best Price, First Time)
    ems.spscQueue(0).enqueue(makeOrder(1, 401, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
    // Seller 2 places order at $100 (Best Price, Second Time)
    ems.spscQueue(0).enqueue(makeOrder(2, 402, 2, 0, Side::SELL, OrderType::LIMIT, 100, 50));
    // Seller 1 places order at $101 (Worse Price)
    ems.spscQueue(0).enqueue(makeOrder(3, 403, 1, 0, Side::SELL, OrderType::LIMIT, 101, 50));

    // Buyer 3 places Market Order for 120 shares
    ems.spscQueue(0).enqueue(makeOrder(4, 404, 3, 0, Side::BUY, OrderType::MARKET, 0, 120));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

    std::lock_guard lock(mockWriter.mutex);
    
    ASSERT_EQ(mockWriter.trades.size(), 3u);
    
    // Trade 1: Seller 1 gets filled first (Time Priority)
    EXPECT_EQ(mockWriter.trades[0].seller_order_id, 401);
    EXPECT_EQ(mockWriter.trades[0].qty, 50);

    // Trade 2: Seller 2 gets filled second (Time Priority)
    EXPECT_EQ(mockWriter.trades[1].seller_order_id, 402);
    EXPECT_EQ(mockWriter.trades[1].qty, 50);

    // Trade 3: Seller 1 gets partially filled at worse price (Price Priority)
    EXPECT_EQ(mockWriter.trades[2].seller_order_id, 403);
    EXPECT_EQ(mockWriter.trades[2].qty, 20); // Only needs 20 to finish the 120 order
    EXPECT_EQ(mockWriter.trades[2].price, 101);

    // State Checks
    EXPECT_EQ(mockWriter.orders[401], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[402], OrderState::FILLED);
    EXPECT_EQ(mockWriter.orders[403], OrderState::PARTIALLY_FILLED);
    EXPECT_EQ(mockWriter.orders[404], OrderState::FILLED); 
}

// ============================================================================
// TEST 5: High-Throughput Engine Determinism (Load Test)
// ============================================================================
TEST_F(ExhaustiveTradingTest, HighThroughputDeterminismNoDataLoss)
{
    EMSCore ems(1, 1, symbolCache);
    ems.balanceCache().setHoldings(1, 0, 1000000, 0); 
    ems.balanceCache().setBalance(2, 100000000, 0);   

    MockPGWriter mockWriter;
    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
    ems.start(); dbWriter.start();

    const int ORDER_COUNT = 10000;

    // Push 10,000 orders as fast as possible to verify the SPSC Queues and RingBuffers 
    // do not drop or corrupt data under load.
    for (int i = 1; i <= ORDER_COUNT; ++i) {
        if (i % 2 != 0) {
            // Odd numbers: Sellers place resting orders
            while(!ems.spscQueue(0).enqueue(makeOrder(i, i, 1, 0, Side::SELL, OrderType::LIMIT, 100, 1)));
        } else {
            // Even numbers: Buyers place marketable limit orders
            while(!ems.spscQueue(0).enqueue(makeOrder(i, i, 2, 0, Side::BUY, OrderType::LIMIT, 100, 1)));
        }
    }

    // Give DBWriter time to pull from the lock-free queues
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

    std::lock_guard lock(mockWriter.mutex);
    
    // 5,000 Sellers and 5,000 Buyers should result in exactly 5,000 trades.
    EXPECT_EQ(mockWriter.trades.size(), 5000u);
    
    // Every single order should be marked as FILLED. Zero dropped messages.
    EXPECT_EQ(mockWriter.orders.size(), 10000u);
}
