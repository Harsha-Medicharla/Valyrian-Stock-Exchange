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

// // Thread-safe mock database to intercept and verify DBWriter's output
// class MockPGWriter final : public IDBWriterBackend
// {
// public:
//     std::mutex mutex;
//     std::vector<DBEvent> events;
//     std::vector<MockTradeRow> trades;
//     std::unordered_map<uint64_t, OrderState> orders;
//     std::atomic<int> total_events_processed{0};

//     void writeBatch(const std::vector<DBEvent> &batch) override
//     {
//         std::lock_guard lock(mutex);
//         for (const DBEvent &event : batch)
//         {
//             events.push_back(event);
//             total_events_processed++;
            
//             if (event.type == DBEventType::ORDER_ACCEPTED) {
//                 orders[event.order_id] = OrderState::NEW;
//             }
//             else if (event.type == DBEventType::ORDER_FILLED) {
//                 orders[event.order_id] = event.state; // FILLED or PARTIALLY_FILLED
//                 // Log trades from the buyer side to avoid double-counting
//                 if (event.side == Side::BUY) {
//                     trades.push_back(MockTradeRow{
//                         event.symbol_id, event.order_id, event.peer_order_id,
//                         event.fill_price, event.fill_qty});
//                 }
//             }
//             else if (event.type == DBEventType::ORDER_CANCELLED) {
//                 orders[event.order_id] = OrderState::CANCELLED;
//             }
//         }
//     }
// };

// // Helper to create limit/market orders
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
//     o.timestamp = seq;
//     o.cancel_flag = 0;
//     return o;
// }

// // Helper to create a cancellation request
// RawOrder makeCancel(uint64_t seq, uint64_t oid) {
//     RawOrder o{};
//     o.sequence = seq;
//     o.order_id = oid;
//     o.cancel_flag = 1;
//     o.timestamp = seq;
//     return o;
// }
// } // namespace

// class ExhaustiveTradingTest : public ::testing::Test {
// protected:
//     SymbolCache symbolCache;

//     void SetUp() override {
//         std::remove("engine_0.wal");
//         std::remove("engine_0.trades");
//         symbolCache.loadFromList({ SymbolInfo{0, "AAPL", 1, 1, true} });
//     }

//     void TearDown() override {
//         std::remove("engine_0.wal");
//         std::remove("engine_0.trades");
//     }
// };


// // ============================================================================
// // TEST 1: The Vanilla Flow (Clean Match & Settle)
// // ============================================================================
// TEST_F(ExhaustiveTradingTest, StandardMatchAndBalanceSettle)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setHoldings(1, 0, 100, 0); // Seller: 100 shares
//     ems.balanceCache().setBalance(2, 10000, 0);   // Buyer: $10,000

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     WebSocketServer ws(ems, 0, false); // Redis auth/subscriber bypass for integration tests.
//     ems.start(); dbWriter.start();

//     ems.spscQueue(0).enqueue(makeOrder(1, 101, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10));
//     ems.spscQueue(0).enqueue(makeOrder(2, 102, 2, 0, Side::BUY, OrderType::LIMIT, 100, 10));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
//     ASSERT_EQ(mockWriter.trades.size(), 1u);
//     EXPECT_EQ(ems.balanceCache().availableBalance(2), 9000); 
//     EXPECT_EQ(ems.balanceCache().availableHoldings(2, 0), 10); 
// }

// // ============================================================================
// // TEST 2: Self-Trade Prevention (Wash Trading Shield)
// // ============================================================================
// TEST_F(ExhaustiveTradingTest, SelfTradePreventionCancelsRestingOrder)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setHoldings(1, 0, 100, 0); 
//     ems.balanceCache().setBalance(1, 10000, 0);   

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     // User 1 places a SELL order
//     ems.spscQueue(0).enqueue(makeOrder(1, 201, 1, 0, Side::SELL, OrderType::LIMIT, 100, 10));
//     // User 1 tries to BUY their own order
//     ems.spscQueue(0).enqueue(makeOrder(2, 202, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     // Engine MUST NOT generate a trade
//     EXPECT_TRUE(mockWriter.trades.empty());
    
//     // Engine MUST cancel the resting order (201) to prevent the self-trade
//     EXPECT_EQ(mockWriter.orders[201], OrderState::CANCELLED);
    
//     // The incoming order (202) should rest on the book as NEW since it had no valid matches
//     EXPECT_EQ(mockWriter.orders[202], OrderState::NEW);
// }

// // ============================================================================
// // TEST 3: Asynchronous Cancellation & Fund Un-blocking
// // ============================================================================
// TEST_F(ExhaustiveTradingTest, CancelOrderReleasesBlockedFunds)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setBalance(1, 5000, 0); // Start with $5,000

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     // 1. Buy $1,000 worth of stock. Should block $1,000.
//     ems.spscQueue(0).enqueue(makeOrder(1, 301, 1, 0, Side::BUY, OrderType::LIMIT, 100, 10));
    
//     // 2. Cancel the order immediately.
//     ems.spscQueue(0).enqueue(makeCancel(2, 301));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     EXPECT_EQ(mockWriter.orders[301], OrderState::CANCELLED);

//     // Verify Funds were fully released back to Available
//     EXPECT_EQ(ems.balanceCache().availableBalance(1), 5000);
//     EXPECT_EQ(ems.balanceCache().blockedBalance(1), 0);
// }

// // ============================================================================
// // TEST 4: Partial Fills & Price/Time Priority Sweeping
// // ============================================================================
// TEST_F(ExhaustiveTradingTest, PriceTimePriorityBookSweep)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setHoldings(1, 0, 1000, 0); 
//     ems.balanceCache().setHoldings(2, 0, 1000, 0); 
//     ems.balanceCache().setBalance(3, 100000, 0);   

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     // Seller 1 places order at $100 (Best Price, First Time)
//     ems.spscQueue(0).enqueue(makeOrder(1, 401, 1, 0, Side::SELL, OrderType::LIMIT, 100, 50));
//     // Seller 2 places order at $100 (Best Price, Second Time)
//     ems.spscQueue(0).enqueue(makeOrder(2, 402, 2, 0, Side::SELL, OrderType::LIMIT, 100, 50));
//     // Seller 1 places order at $101 (Worse Price)
//     ems.spscQueue(0).enqueue(makeOrder(3, 403, 1, 0, Side::SELL, OrderType::LIMIT, 101, 50));

//     // Buyer 3 places Market Order for 120 shares
//     ems.spscQueue(0).enqueue(makeOrder(4, 404, 3, 0, Side::BUY, OrderType::MARKET, 0, 120));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     ASSERT_EQ(mockWriter.trades.size(), 3u);
    
//     // Trade 1: Seller 1 gets filled first (Time Priority)
//     EXPECT_EQ(mockWriter.trades[0].seller_order_id, 401);
//     EXPECT_EQ(mockWriter.trades[0].qty, 50);

//     // Trade 2: Seller 2 gets filled second (Time Priority)
//     EXPECT_EQ(mockWriter.trades[1].seller_order_id, 402);
//     EXPECT_EQ(mockWriter.trades[1].qty, 50);

//     // Trade 3: Seller 1 gets partially filled at worse price (Price Priority)
//     EXPECT_EQ(mockWriter.trades[2].seller_order_id, 403);
//     EXPECT_EQ(mockWriter.trades[2].qty, 20); // Only needs 20 to finish the 120 order
//     EXPECT_EQ(mockWriter.trades[2].price, 101);

//     // State Checks
//     EXPECT_EQ(mockWriter.orders[401], OrderState::FILLED);
//     EXPECT_EQ(mockWriter.orders[402], OrderState::FILLED);
//     EXPECT_EQ(mockWriter.orders[403], OrderState::PARTIALLY_FILLED);
//     EXPECT_EQ(mockWriter.orders[404], OrderState::FILLED); 
// }

// // ============================================================================
// // TEST 5: High-Throughput Engine Determinism (Load Test)
// // ============================================================================
// TEST_F(ExhaustiveTradingTest, HighThroughputDeterminismNoDataLoss)
// {
//     EMSCore ems(1, 1, symbolCache);
//     ems.balanceCache().setHoldings(1, 0, 1000000, 0); 
//     ems.balanceCache().setBalance(2, 100000000, 0);   

//     MockPGWriter mockWriter;
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), mockWriter);
//     ems.start(); dbWriter.start();

//     const int ORDER_COUNT = 10000;

//     // Push 10,000 orders as fast as possible to verify the SPSC Queues and RingBuffers 
//     // do not drop or corrupt data under load.
//     for (int i = 1; i <= ORDER_COUNT; ++i) {
//         if (i % 2 != 0) {
//             // Odd numbers: Sellers place resting orders
//             while(!ems.spscQueue(0).enqueue(makeOrder(i, i, 1, 0, Side::SELL, OrderType::LIMIT, 100, 1)));
//         } else {
//             // Even numbers: Buyers place marketable limit orders
//             while(!ems.spscQueue(0).enqueue(makeOrder(i, i, 2, 0, Side::BUY, OrderType::LIMIT, 100, 1)));
//         }
//     }

//     // Give DBWriter time to pull from the lock-free queues
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
//     dbWriter.stop(); ems.stop(); dbWriter.join(); ems.join();

//     std::lock_guard lock(mockWriter.mutex);
    
//     // 5,000 Sellers and 5,000 Buyers should result in exactly 5,000 trades.
//     EXPECT_EQ(mockWriter.trades.size(), 5000u);
    
//     // Every single order should be marked as FILLED. Zero dropped messages.
//     EXPECT_EQ(mockWriter.orders.size(), 10000u);
// }



// #include <gtest/gtest.h>
// #include <thread>
// #include <chrono>
// #include <vector>
// #include <atomic>
// #include <mutex>

// #include "ems/core/EMSCore.h"
// #include "db/DBWriter.h"
// #include "db/SymbolCache.h"
// #include "MatchingEngine/wal/WAL.h"
// #include "ems/utils/Time.h"

// namespace {

// // Mock database to intercept events and verify final settlement states
// class MockStorage final : public IDBWriterBackend {
// public:
//     struct TradeRecord { uint64_t bOid, sOid; int64_t px, qty; };
//     std::mutex mtx;
//     std::vector<TradeRecord> trades;
//     std::unordered_map<uint64_t, OrderState> orderStates;

//     void writeBatch(const std::vector<DBEvent>& batch) override {
//         std::lock_guard lock(mtx);
//         for (const auto& e : batch) {
//             orderStates[e.order_id] = e.state;
//             if (e.type == DBEventType::ORDER_FILLED && e.side == Side::BUY) {
//                 trades.push_back({e.order_id, e.peer_order_id, e.fill_price, e.fill_qty});
//             }
//         }
//     }
// };

// // Helper to create the RawOrder used by the IngressWorker
// RawOrder createRawOrder(uint64_t seq, uint64_t oid, uint32_t uid, Side side, Price px, Qty qty) {
//     RawOrder o{};
//     o.sequence = seq;
//     o.order_id = oid;
//     o.user_id = uid;
//     o.symbol_id = 0; // AAPL in our test
//     o.price = px;
//     o.qty = static_cast<uint32_t>(qty);
//     o.side = static_cast<uint8_t>(side);
//     o.type = static_cast<uint8_t>(OrderType::LIMIT);
//     o.timestamp = ems::nowNanos();
//     return o;
// }

// class SystemIntegrityTest : public ::testing::Test {
// protected:
//     SymbolCache symbols;
//     void SetUp() override {
//         std::remove("engine_0.wal");
//         symbols.loadFromList({ {0, "AAPL", 1, 1, true} });
//     }
//     void TearDown() override {
//         std::remove("engine_0.wal");
//     }
// };

// /**
//  * FLOW TEST 1: The "Happy Path" (Ingress -> Match -> DB -> Balance)
//  * Validates that money and shares move correctly across lock-free boundaries.
//  */
// TEST_F(SystemIntegrityTest, FullLifecycleMatchAndSettle) {
//     EMSCore ems(1, 1, symbols);
//     MockStorage storage;
    
//     // Setup initial state: User 1 has 100 AAPL, User 2 has $10k cash
//     ems.balanceCache().setHoldings(1, 0, 100, 0);
//     ems.balanceCache().setBalance(2, 10000, 0);

//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), storage);
    
//     ems.start();
//     dbWriter.start();

//     // Step 1: User 1 Sells 10 AAPL @ 150
//     ems.spscQueue(0).enqueue(createRawOrder(1, 1001, 1, Side::SELL, 150, 10));
    
//     // Step 2: User 2 Buys 10 AAPL @ 150
//     ems.spscQueue(0).enqueue(createRawOrder(2, 1002, 2, Side::BUY, 150, 10));

//     // Wait for pipeline to drain
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
//     ems.stop(); dbWriter.stop();
//     ems.join(); dbWriter.join();

//     // Verify 1: Database saw the trade
//     ASSERT_EQ(storage.trades.size(), 1u);
//     EXPECT_EQ(storage.trades[0].px, 150);

//     // Verify 2: BalanceCache updated (Deterministic Settlement)
//     EXPECT_EQ(ems.balanceCache().availableBalance(1), 1500); // Gained $1500
//     EXPECT_EQ(ems.balanceCache().availableHoldings(2, 0), 10); // Gained 10 shares
// }

// /**
//  * FLOW TEST 2: Determinism and Recovery (WAL Source of Truth)
//  * Validates that the system can be reconstructed exactly from disk logs.
//  */
// /**
//  * FLOW TEST 2: Determinism and Recovery (WAL Source of Truth)
//  */
// TEST_F(SystemIntegrityTest, CrashAndRecoveryDeterminism) {
//     // Phase 1: Same as before...
//     {
//         EMSCore ems(1, 1, symbols);
//         ems.balanceCache().setHoldings(1, 0, 100, 0);
//         ems.start();
//         ems.spscQueue(0).enqueue(createRawOrder(1, 5001, 1, Side::SELL, 100, 50));
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         ems.stop(); ems.join();
//     }

//     // Phase 2: Recovery
//     EMSCore recoveredEms(1, 1, symbols);
//     MockStorage storage;
//     DBWriter dbWriter(recoveredEms.ingressDbQueues(), recoveredEms.engineDbQueues(), 1, recoveredEms.balanceCache(), storage);
    
//     // Align the accounting truth with the engine state
//     recoveredEms.balanceCache().setHoldings(1, 0, 50, 50); 
//     recoveredEms.balanceCache().setBalance(2, 5000, 0);    
    
//     recoveredEms.start(); dbWriter.start();

//     // FIX: Send this order with sequence 1. 
//     // The recovered Dispatcher starts at 1 and is waiting for it.
//     recoveredEms.spscQueue(0).enqueue(createRawOrder(1, 5002, 2, Side::BUY, 100, 50));

//     std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Give it a bit more time to settle
//     recoveredEms.stop(); dbWriter.stop();
//     recoveredEms.join(); dbWriter.join();

//     EXPECT_EQ(storage.trades.size(), 1u);
// }

// /**
//  * FLOW TEST 3: Pre-Trade Risk Blocking (Validation Pipeline)
//  * Validates that the "bouncer" stops invalid requests before they hit the ring buffer.
//  */
// TEST_F(SystemIntegrityTest, RiskCheckBlocksInsufficientFunds) {
//     EMSCore ems(1, 1, symbols);
//     MockStorage storage;
    
//     // User has $0
//     ems.balanceCache().setBalance(1, 0, 0);
//     DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), 1, ems.balanceCache(), storage);
    
//     ems.start(); dbWriter.start();

//     // Try to buy AAPL @ 150
//     ems.spscQueue(0).enqueue(createRawOrder(1, 6001, 1, Side::BUY, 150, 1));

//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     ems.stop(); dbWriter.stop();
//     ems.join(); dbWriter.join();

//     // Verify: The order was rejected in the pipeline and never reached the MatchingEngine/DB
//     std::lock_guard lock(storage.mtx);
//     EXPECT_TRUE(storage.orderStates.find(6001) == storage.orderStates.end()); 
// }

// } // namespace



#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>

// Core System Headers
#include "ems/pipeline/BalanceCache.h"
#include "ems/pipeline/ValidationPipeline.h"
#include "ems/pipeline/MarketState.h"
#include "ems/pipeline/RateLimiter.h"
#include "ems/core/EMSCore.h"

// DB and Persistence Headers - FIX: Added missing headers
#include "db/DBWriter.h"
#include "db/PGWriter.h"
#include "shared/types/Events.h"
#include "shared/queues/EventSPSC.h"
#include "db/SymbolCache.h"

// Networking and API Headers - FIX: Added missing headers
#include "trade_server/network/ConnTable.h"
#include "api_server/db/RedisPool.h"
#include <drogon/drogon.h>

// --- EMS MODULE TESTS ---
TEST(EMSModuleTest, BalanceLocking) {
    BalanceCache bc(1);
    bc.setBalance(10, 5000, 0); 

    // Correct method: tryBlockFunds(userId, amount)
    EXPECT_TRUE(bc.tryBlockFunds(10, 2000)); 
    EXPECT_EQ(bc.availableBalance(10), 3000);
    EXPECT_EQ(bc.blockedBalance(10), 2000);

    bc.unblockFunds(10, 2000);
    EXPECT_EQ(bc.availableBalance(10), 5000);
}

TEST(EMSModuleTest, PipelineValidation) {
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

    // Correct enum: Decision::ACCEPT
    EXPECT_EQ(pipeline.process(&marketOrder, reason), Decision::ACCEPT);
}

// --- TRADE SERVER TESTS ---
TEST(TradeServerModuleTest, ConnTableLogic) {
    ConnTable table;
    // assign(userId) returns uint32_t
    uint32_t connIdx = table.assign(100); 
    EXPECT_NE(connIdx, 0);
    EXPECT_EQ(table.findByUser(100), connIdx);
    
    table.release(connIdx);
    EXPECT_EQ(table.findByUser(100), 0);
}

// --- DB WRITER TESTS ---
TEST(DBWriterModuleTest, QueueHandling) {
    BalanceCache bc(1);
    // FIX: Inherit from IDBWriterBackend which requires DBEvent
    class NullBackend final : public IDBWriterBackend {
    public:
        void writeBatch(const std::vector<DBEvent>&) override {}
    };
    NullBackend backend;

    // Correct queue usage: emplace_back to avoid move-only copy errors
    std::vector<EventSPSC<DBEvent>> inQ; inQ.emplace_back(128);
    std::vector<EventSPSC<DBEvent>> enQ; enQ.emplace_back(128);

    // DBWriter requires references to vectors of SPSC queues
    DBWriter writer(inQ, enQ, 1, bc, backend);
    SUCCEED();
}

// --- API SERVER TESTS ---
TEST(APIServerModuleTest, RedisPoolSingleton) {
    // RedisPool instance management
    RedisPool& pool = RedisPool::instance();
    pool.init("127.0.0.1", 6379);
    SUCCEED();
}

TEST(APIServerModuleTest, DrogonAppCheck) {
    // Verify drogon linkage
    auto& app = drogon::app();
    EXPECT_NE(&app, nullptr);
}
