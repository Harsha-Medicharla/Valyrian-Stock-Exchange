#include <gtest/gtest.h>
#include "MatchingEngine/include/MatchingEngine.h"
#include "Settlement/core/Settlement.h"

// REMOVE the Pool<Trade> and Pool<Confirmation> lines.
// Just use this one line for the bank:
static ::Settlement global_bank(nullptr, nullptr);

static std::string walPath(int id) {
    return "test_wal_" + std::to_string(id);
}



/*
    before running this file,
 - make everything public in OrderBook.h and MatchingEngine.h
*/



///////////////////////////////////////////////////////////////////////////////////
// ---------------------------independent functions test---------------------------
///////////////////////////////////////////////////////////////////////////////////



// test 1
// Allocate returns valid order
TEST(PoolTest, AllocateOrderReturnsNonNull) {
    OrderBook *book = new OrderBook();

    Order* o = book->requestAllocationOfOrder();
    ASSERT_NE(o, nullptr);
}


// test 2
// Deallocation allows reuse
TEST(PoolTest, DeallocateAndReuseOrder) {
    OrderBook *book = new OrderBook();

    Order* o1 = book->requestAllocationOfOrder();
    ASSERT_NE(o1, nullptr);

    book->requestDeAllocationOfOrder(o1);

    Order* o2 = book->requestAllocationOfOrder();
    ASSERT_NE(o2, nullptr);

    // Pool should reuse memory
    EXPECT_EQ(o1, o2);
}


// test 3
// Multiple allocations are distinct
TEST(PoolTest, MultipleAllocationsDistinct) {
    OrderBook *book = new OrderBook();

    Order* o1 = book->requestAllocationOfOrder();
    Order* o2 = book->requestAllocationOfOrder();

    ASSERT_NE(o1, nullptr);
    ASSERT_NE(o2, nullptr);
    EXPECT_NE(o1, o2);
}


// test 4
// Deallocating nullptr is safe (defensive)
TEST(PoolTest, DeallocateNullptrThrowsLogicError) {
    OrderBook *book = new OrderBook();
    EXPECT_THROW(
        book->requestDeAllocationOfOrder(nullptr),
        std::logic_error
    );
}


// test 5
// Insert + Find
TEST(ARTTest, InsertAndFind) {
    AdaptiveRadixTree tree;
    PriceLevel *level;
    tree.insert(10, level);
    auto val = tree.find(10);

    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val, level);
}


// test 6
// Find non-existent key
TEST(ARTTest, FindMissingKeyReturnsNull) {
    AdaptiveRadixTree tree;

    EXPECT_EQ(tree.find(999), nullptr);
}


// test 7
// Erase removes key
TEST(ARTTest, EraseRemovesKey) {
    AdaptiveRadixTree tree;
    PriceLevel *level;
    tree.insert(5, level);
    tree.erase(5);

    EXPECT_EQ(tree.find(5), nullptr);
}


// test 8
// Multiple inserts preserve correctness
TEST(ARTTest, MultipleKeysWork) {
    AdaptiveRadixTree tree;
    PriceLevel *level1;
    PriceLevel *level2;
    PriceLevel *level3;
    tree.insert(1, level1);
    tree.insert(2, level2);
    tree.insert(3, level3);

    EXPECT_EQ(tree.find(1), level1);
    EXPECT_EQ(tree.find(2), level2);
    EXPECT_EQ(tree.find(3), level3);
}


// test 9
// findOrder
TEST(OrderBookTest, FindOrderById) {
    OrderBook *book = new OrderBook();

    Order* o = book->requestAllocationOfOrder();
    *o = {1,1,Side::BUY,OrderType::LIMIT,10,100,100,0,OrderState::NEW,nullptr,nullptr};

    book->insertOrder(o);

    Order* found = book->findOrder(1);
    EXPECT_EQ(found, o);
}


// test 10
// getOrCreatePriceLevel
TEST(OrderBookTest, GetOrCreatePriceLevel) {
    OrderBook *book = new OrderBook();

    PriceLevel* level = book->getOrCreatePriceLevel(Side::BUY, 10);
    ASSERT_NE(level, nullptr);

    EXPECT_TRUE(book->buy_book.size() == 1);
    EXPECT_TRUE(book->buy_book.find(10) == level);
}


// test 11
// getPriceLevel
TEST(OrderBookTest, GetPriceLevel) {
    OrderBook *book = new OrderBook();

    book->getOrCreatePriceLevel(Side::SELL, 20);
    PriceLevel* level = book->getPriceLevel(Side::SELL, 20);

    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->price, 20);
}


// test 12
// removePriceLevelIfEmpty
TEST(OrderBookTest, RemovePriceLevelIfEmpty) {
    OrderBook *book = new OrderBook();

    PriceLevel* level = book->getOrCreatePriceLevel(Side::BUY, 30);
    ASSERT_NE(level, nullptr);

    book->removePriceLevelIfEmpty(Side::BUY, 30);

    EXPECT_TRUE(book->buy_book.empty());
}


// test 13
// updateOrderState() (MatchingEngine)
TEST(OrderStateTest, UpdateOrderState) {
    MatchingEngine *engine = new MatchingEngine(7,global_bank);

    Order o{};

    o.quantity = 100;
    o.remaining = 100;
    engine->updateOrderState(&o);
    EXPECT_EQ(o.state, OrderState::NEW);

    o.remaining = 50;
    engine->updateOrderState(&o);
    EXPECT_EQ(o.state, OrderState::PARTIALLY_FILLED);

    o.remaining = 0;
    engine->updateOrderState(&o);
    EXPECT_EQ(o.state, OrderState::FILLED);
}


// test 14
// Test FIFO push order
TEST(PriceLevelTest, FIFOPushOrder) {
    PriceLevel level{};

    Order a{}, b{}, c{};

    level.fifoPush(&a);
    level.fifoPush(&b);
    level.fifoPush(&c);

    EXPECT_EQ(level.head, &a);
    EXPECT_EQ(a.next, &b);
    EXPECT_EQ(b.next, &c);
    EXPECT_EQ(level.tail, &c);
}


// test 15
// Test FIFO remove order
TEST(PriceLevelTest, FIFORemoveMiddle) {
    PriceLevel level{};
    Order a{}, b{}, c{};

    level.fifoPush(&a);
    level.fifoPush(&b);
    level.fifoPush(&c);

    level.fifoRemove(&b);

    EXPECT_EQ(a.next, &c);
    EXPECT_EQ(c.prev, &a);
    EXPECT_EQ(level.head, &a);
    EXPECT_EQ(level.tail, &c);
}


// test 16
// consumeOrder() (OrderBook)
TEST(OrderBookTest, ConsumeOrder) {
    OrderBook *book = new OrderBook();

    Order o{};
    o.side = Side::BUY;        // or SELL
    o.price = 100;
    o.remaining = 100;

    // Create price level first
    PriceLevel* level = book->getOrCreatePriceLevel(o.side, o.price);
    level->aggregated_qty = 100;

    book->consumeOrder(&o, 40);

    EXPECT_EQ(o.remaining, 60);
    EXPECT_EQ(level->aggregated_qty, 60);
}



///////////////////////////////////////////////////////////////////////////////////
// -----------------functions that call independent functions test-----------------
///////////////////////////////////////////////////////////////////////////////////



// test 1
// insertOrder()
TEST(OrderBookTest, InsertOrderCreatesPriceLevel) {
    OrderBook *book = new OrderBook();

    Order* o = book->requestAllocationOfOrder();
    *o = {1, 1, Side::BUY, OrderType::LIMIT, 10, 100, 100, 0, OrderState::NEW, nullptr, nullptr};

    book->insertOrder(o);

    ASSERT_EQ(book->buy_book.size(), 1);
    EXPECT_EQ(book->buy_book.find(10)->aggregated_qty, 100);
}


// test 2
// removeOrder()
TEST(OrderBookTest, RemoveOrderDeletesPriceLevel) {
    OrderBook *book = new OrderBook();

    Order* o = book->requestAllocationOfOrder();
    *o = {1, 1, Side::SELL, OrderType::LIMIT, 20, 50, 50, 0, OrderState::NEW, nullptr, nullptr};

    book->insertOrder(o);
    book->removeOrder(o);

    EXPECT_TRUE(book->sell_book.empty());
}


// test 3
// getOrderAtBestPrice()
TEST(OrderBookTest, GetOrderAtBestPrice) {
    OrderBook *book = new OrderBook();

    Order* o1 = book->requestAllocationOfOrder();
    *o1 = {1,1,Side::SELL,OrderType::LIMIT,20,50,50,0,OrderState::NEW,nullptr,nullptr};

    Order* o2 = book->requestAllocationOfOrder();
    *o2 = {2,1,Side::SELL,OrderType::LIMIT,10,50,50,0,OrderState::NEW,nullptr,nullptr};

    book->insertOrder(o1);
    book->insertOrder(o2);

    Order* best = book->getOrderAtBestPrice(Side::SELL);
    EXPECT_EQ(best->price, 10);
}


// test 4
// match() (without crossing)
TEST(MatchingEngineUnitTest, MatchStopsOnNoCross) {
    MatchingEngine *engine = new MatchingEngine(8,global_bank);

    engine->onNewOrder(1,1,Side::BUY,OrderType::LIMIT,10,100,0);

    Order* buy = engine->order_book.findOrder(1);
    ASSERT_NE(buy, nullptr);
    EXPECT_EQ(buy->remaining, 100);
}


// test 5
// executeTrade() (WAL side-effect only)
TEST(MatchingEngineUnitTest, ExecuteTradeDoesNotMutateOrders) {
    MatchingEngine *engine = new MatchingEngine(9,global_bank);

    Order a{}, b{};
    a.remaining = 50;
    b.remaining = 50;

    engine->executeTrade(&a, &b, 10, 20);

    EXPECT_EQ(a.remaining, 50);
    EXPECT_EQ(b.remaining, 50);
}


// test 6
// onNewOrder()
TEST(MatchingEngineUnitTest, OnNewOrderRestingLimit) {
    MatchingEngine *engine = new MatchingEngine(10,global_bank);

    bool ok = engine->onNewOrder(1,1,Side::BUY,OrderType::LIMIT,10,100,0);

    EXPECT_TRUE(ok);
    EXPECT_EQ(engine->order_book.buy_book.size(), 1);
}


// test 7
// Cancel filled order
TEST(MatchingEngineUnitTest, CancelFilledOrderFails) {
    MatchingEngine *engine = new MatchingEngine(11,global_bank);

    Order* o = engine->order_book.requestAllocationOfOrder();
    *o = {1,1,Side::BUY,OrderType::LIMIT,10,0,0,0,OrderState::FILLED,nullptr,nullptr};

    bool ok = engine->onCancelOrder(1);
    EXPECT_FALSE(ok);
}


// test 8
// Modify non-existent order
TEST(MatchingEngineUnitTest, ModifyNonExistentOrder) {
    MatchingEngine *engine = new MatchingEngine(12  ,global_bank);
    EXPECT_FALSE(engine->onModifyOrder(999, 10, 100));
}


// test 9
// Modify filled order fails safely
TEST(MatchingEngineTest, ModifyFilledOrderFails) {
    MatchingEngine *engine = new MatchingEngine(13,global_bank);

    Order* o = engine->order_book.requestAllocationOfOrder();
    *o = {1,1,Side::BUY,OrderType::LIMIT,10,0,0,0,OrderState::FILLED,nullptr,nullptr};

    EXPECT_FALSE(engine->onModifyOrder(1, 20, 100));
}


// test 10
// Reduce quantity only (same price)

TEST(MatchingEngineTest, ModifyReduceQuantitySamePrice) {
    MatchingEngine *engine = new MatchingEngine(14,global_bank);

    engine->onNewOrder(1,1,Side::BUY,OrderType::LIMIT,10,100,0);

    bool ok = engine->onModifyOrder(1, 10, 60);
    EXPECT_TRUE(ok);

    Order* updated = engine->order_book.findOrder(1);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->quantity, 60);
    EXPECT_EQ(updated->remaining, 60);
}


// test 11
// Modify causes cancel + reinsert
TEST(MatchingEngineTest, ModifyPriceCausesReinsert) {
    MatchingEngine *engine = new MatchingEngine(15,global_bank);

    engine->onNewOrder(1,1,Side::BUY,OrderType::LIMIT,10,100,0);

    bool ok = engine->onModifyOrder(1, 20, 200);
    EXPECT_TRUE(ok);

    EXPECT_EQ(engine->order_book.buy_book.size(), 1);
    EXPECT_NE(engine->order_book.buy_book.find(20), nullptr);
}



// ///////////////////////////////////////////////////////////////////////////
// -------------------------------logical tests-------------------------------
// ///////////////////////////////////////////////////////////////////////////



// test 1
/*
What this test guarantees
	•	Deterministic clean startup
	•	No garbage price levels
	•	No accidental pre-allocation
	•	Pool is untouched
*/
TEST(MatchingEngineTest, EmptyBookOnStartup) {
    MatchingEngine *engine = new MatchingEngine(16,global_bank);
    // Buy & sell ladders must be empty
    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.sell_book.empty());

    // No best bid / ask
    EXPECT_TRUE(engine->order_book.best_bid == nullptr);
    EXPECT_TRUE(engine->order_book.best_ask == nullptr);
}


// test 2
/*
What this test validates
	•	Pool allocation works
	•	Order insertion path is correct
	•	Buy ladder populated
	•	Aggregated quantity correct
	•	FIFO pointers correct
	•	No accidental matching
*/
TEST(MatchingEngineTest, InsertBuyLimitNoMatch) {
    MatchingEngine *engine = new MatchingEngine(17,global_bank);

    engine->onNewOrder(1,42,Side::BUY,OrderType::LIMIT,10,100,1);

    ASSERT_EQ(engine->order_book.buy_book.size(), 1);

    Order* o = engine->order_book.findOrder(1);
    ASSERT_NE(o, nullptr);

    PriceLevel* level = engine->order_book.buy_book.find(10);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 100);
    EXPECT_EQ(level->head, o);
    EXPECT_EQ(level->tail, o);
    EXPECT_EQ(engine->order_book.best_bid, level);
}


// test 3
/*
This test mirrors Test 2 but on the sell side, and it validates:
	•	Sell ladder insertion
	•	Best ask update
	•	No accidental matching
	•	FIFO correctness on sell side
*/
TEST(MatchingEngineTest, InsertSellLimitNoMatch) {
    MatchingEngine *engine = new MatchingEngine(18,global_bank);

    engine->onNewOrder(2,99,Side::SELL,OrderType::LIMIT,20,150,1);

    ASSERT_EQ(engine->order_book.sell_book.size(), 1);

    Order* o = engine->order_book.findOrder(2);
    ASSERT_NE(o, nullptr);

    PriceLevel* level = engine->order_book.sell_book.find(20);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 150);
    EXPECT_EQ(level->head, o);
    EXPECT_EQ(level->tail, o);
    EXPECT_EQ(engine->order_book.best_ask, level);
}


// test 4
/*
What this test validates
	•	Crossing logic (>= / <=)
	•	Matching loop
	•	Quantity decrement
	•	Order state transition
	•	Price level cleanup
	•	Pool deallocation safety
*/
TEST(MatchingEngineTest, ExactPriceCrossFullFill) {
    MatchingEngine *engine = new MatchingEngine(19,global_bank);

    engine->onNewOrder(1,10,Side::SELL,OrderType::LIMIT,10,100,1);
    engine->onNewOrder(2,20,Side::BUY,OrderType::LIMIT,10,100,2);

    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.sell_book.empty());
    EXPECT_TRUE(engine->order_book.best_bid == nullptr);
    EXPECT_TRUE(engine->order_book.best_ask == nullptr);

    // FILLED orders must not exist anymore (deallocated)
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);
    EXPECT_TRUE(engine->order_book.findOrder(2) == nullptr);
}


// test 5
/*
This test validates that:
	•	Only part of a resting order is consumed
	•	Remaining quantity stays on the book
	•	FIFO pointers remain correct
	•	Best bid / ask is preserved
*/
TEST(MatchingEngineTest, PartialFillSingleLevel) {
    MatchingEngine *engine = new MatchingEngine(20,global_bank);

    // Insert SELL
    engine->onNewOrder(1, 10, Side::SELL, OrderType::LIMIT, 10, 200, 1);

    // Insert BUY
    engine->onNewOrder(2, 20, Side::BUY, OrderType::LIMIT, 10, 100, 2);

    // BUY must be deallocated (FILLED)
    EXPECT_TRUE(engine->order_book.findOrder(2) == nullptr);

    // SELL must still exist
    Order* sell = engine->order_book.findOrder(1);
    ASSERT_NE(sell, nullptr);
    EXPECT_EQ(sell->remaining, 100);

    ASSERT_EQ(engine->order_book.sell_book.size(), 1);
    PriceLevel* level = engine->order_book.sell_book.find(10);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 100);
    EXPECT_EQ(level->head, sell);
    EXPECT_EQ(level->tail, sell);
    EXPECT_EQ(engine->order_book.best_ask, level);
    EXPECT_TRUE(engine->order_book.buy_book.empty());
}


// test 6
/*
What this test validates
	•	Intrusive FIFO list correctness
	•	fifoPush order
	•	fifo_remove correctness
	•	Matching loop respects FIFO
	•	No pointer corruption
*/
TEST(MatchingEngineTest, FIFOAtSamePriceLevel) {
    MatchingEngine *engine = new MatchingEngine(21,global_bank);

    engine->onNewOrder(1, 10, Side::SELL, OrderType::LIMIT, 10, 100, 1);
    engine->onNewOrder(2, 20, Side::SELL, OrderType::LIMIT, 10, 100, 2);
    engine->onNewOrder(3, 30, Side::BUY,  OrderType::LIMIT, 10, 150, 3);

    // First SELL must be gone
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);

    // BUY must be gone
    EXPECT_TRUE(engine->order_book.findOrder(3) == nullptr);

    // Second SELL must remain
    Order* sell2 = engine->order_book.findOrder(2);
    ASSERT_NE(sell2, nullptr);
    EXPECT_EQ(sell2->remaining, 50);

    ASSERT_EQ(engine->order_book.sell_book.size(), 1);
    PriceLevel* level = engine->order_book.sell_book.find(10);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 50);
    EXPECT_EQ(level->head, sell2);
    EXPECT_EQ(level->tail, sell2);
}


// test 7
/*
What this test validates
	•	Correct best-price selection
	•	Ladder traversal logic
	•	best_ask updates
	•	No FIFO leakage across price levels
*/
TEST(MatchingEngineTest, PricePriorityAcrossLevels) {
    MatchingEngine *engine = new MatchingEngine(22,global_bank);

    engine->onNewOrder(1, 10, Side::SELL, OrderType::LIMIT, 10, 100, 1);
    engine->onNewOrder(2, 20, Side::SELL, OrderType::LIMIT, 9, 100, 2);
    engine->onNewOrder(3, 30, Side::BUY,  OrderType::LIMIT, 10, 150, 3);

    // SELL @ 9 must be gone
    EXPECT_TRUE(engine->order_book.findOrder(2) == nullptr);

    // BUY must be gone
    EXPECT_TRUE(engine->order_book.findOrder(3) == nullptr);

    // SELL @ 10 must remain
    Order* sell10 = engine->order_book.findOrder(1);
    ASSERT_NE(sell10, nullptr);
    EXPECT_EQ(sell10->remaining, 50);

    ASSERT_EQ(engine->order_book.sell_book.size(), 1);
    PriceLevel* level = engine->order_book.sell_book.find(10);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 50);
    EXPECT_EQ(engine->order_book.best_ask, level);
}


// test 8
/*
What this test validates
	•	Market order path
	•	No insertion into price ladder
	•	Correct best-price traversal
	•	Correct partial fill at second level
	•	Book integrity after sweep
*/
TEST(MatchingEngineTest, MarketOrderSweep) {
    MatchingEngine *engine = new MatchingEngine(23,global_bank);

    engine->onNewOrder(1, 10, Side::SELL, OrderType::LIMIT, 10, 100, 1);
    engine->onNewOrder(2, 20, Side::SELL, OrderType::LIMIT, 11, 100, 2);

    engine->onNewOrder(3, 30, Side::BUY, OrderType::MARKET, 0, 150, 3);

    // Market order must be deallocated
    EXPECT_TRUE(engine->order_book.findOrder(3) == nullptr);

    // SELL @10 must be gone
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);

    // SELL @11 must remain partially filled
    Order* sell11 = engine->order_book.findOrder(2);
    ASSERT_NE(sell11, nullptr);
    EXPECT_EQ(sell11->remaining, 50);

    ASSERT_EQ(engine->order_book.sell_book.size(), 1);
    PriceLevel* level = engine->order_book.sell_book.find(11);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 50);
    EXPECT_EQ(engine->order_book.best_ask, level);
}


// test 9
/*
it must:
	•	Remove the order from FIFO correctly
	•	Update aggregated quantity
	•	Delete price level if empty
	•	Update best bid / ask
	•	Not corrupt the pool

Expected behavior
	•	Order removed from book
	•	Buy ladder becomes empty
	•	best_bid reset
	•	Order remaining unchanged (or zero, depending on design)
	•	No crash, no dangling pointers
*/
TEST(MatchingEngineTest, CancelRestingOrder) {
    MatchingEngine *engine = new MatchingEngine(24,global_bank);

    engine->onNewOrder(1, 10, Side::BUY, OrderType::LIMIT, 10, 100, 1);

    ASSERT_EQ(engine->order_book.buy_book.size(), 1);

    engine->onCancelOrder(1);

    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.best_bid == nullptr);
    EXPECT_TRUE(engine->order_book.sell_book.empty());

    // Order must be removed
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);
}


// test 10
/*
This test ensures:
	•	Old order is fully removed
	•	New order is inserted correctly
	•	FIFO priority is reset
	•	New price level is respected
*/
TEST(MatchingEngineTest, ModifyOrderCancelAndReinsert) {
    MatchingEngine *engine = new MatchingEngine(25,global_bank);

    engine->onNewOrder(1, 10, Side::BUY, OrderType::LIMIT, 10, 100, 1);

    ASSERT_EQ(engine->order_book.buy_book.size(), 1);

    engine->onModifyOrder(1, 11, 200);

    EXPECT_TRUE(engine->order_book.buy_book.find(10) == nullptr);

    ASSERT_EQ(engine->order_book.buy_book.size(), 1);

    PriceLevel* level = engine->order_book.buy_book.find(11);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 200);

    Order* modified = level->head;
    ASSERT_NE(modified, nullptr);

    EXPECT_EQ(modified->order_id, 1);
    EXPECT_EQ(modified->remaining, 200);
    EXPECT_EQ(engine->order_book.best_bid, level);
}


// test 11
// test to cancel non-existing orders
TEST(MatchingEngineTest, CancelNonExistentOrder) {
    MatchingEngine *engine = new MatchingEngine(26,global_bank);
    engine->onCancelOrder(999);  // should not crash
    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.sell_book.empty());
}


// test 12
// test to place market order on empty order book
TEST(MatchingEngineTest, MarketOrderOnEmptyBook) {
    MatchingEngine *engine = new MatchingEngine(27,global_bank);

    engine->onNewOrder(1, 10, Side::BUY, OrderType::MARKET, 0, 100, 1);

    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.sell_book.empty());

    // Must be immediately deallocated
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);
}



//////////////////////////////////////////////////////////////////////////////
// ---------------------------------wal tests---------------------------------
//////////////////////////////////////////////////////////////////////////////



// test 1
// logInput() — ADD correctness
TEST(WALTest, LogInputWritesAddEntry) {
    WALSystem *wal = new WALSystem(walPath(1));

    Order o{};
    o.order_id = 1;
    o.user_id = 42;
    o.side = Side::BUY;
    o.type = OrderType::LIMIT;
    o.price = 10;
    o.quantity = 100;
    o.remaining = 100;
    o.timestamp = 5;
    o.state = OrderState::NEW;

    wal->logInput(WalAction::ADD, &o);

    std::vector<LogEntry> entries;
    wal->recover([&](const LogEntry& e) {
        entries.push_back(e);
    });

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].action, WalAction::ADD);
    EXPECT_EQ(entries[0].data.order_id, 1);
    EXPECT_EQ(entries[0].data.price, 10);
    EXPECT_EQ(entries[0].data.quantity, 100);
}


// test 2
// logModify() — MODIFY correctness
TEST(WALTest, LogModifyWritesModifyEntry) {
    WALSystem *wal = new WALSystem(walPath(2));

    wal->logModify(7, 25, 300);

    std::vector<LogEntry> entries;
    wal->recover([&](const LogEntry& e) {
        entries.push_back(e);
    });

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].action, WalAction::MODIFY);
    EXPECT_EQ(entries[0].data.order_id, 7);
    EXPECT_EQ(entries[0].data.price, 25);
    EXPECT_EQ(entries[0].data.quantity, 300);
}


// test 3
// logCancel() — CANCEL correctness
TEST(WALTest, LogCancelWritesCancelEntry) {
    WALSystem *wal = new WALSystem(walPath(3));

    wal->logCancel(99);

    std::vector<LogEntry> entries;
    wal->recover([&](const LogEntry& e) {
        entries.push_back(e);
    });

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].action, WalAction::CANCEL);
    EXPECT_EQ(entries[0].data.order_id, 99);
}


// test 4
// WAL append behavior (multiple calls)
TEST(WALTest, MultipleEntriesAppendInOrder) {
    WALSystem *wal = new WALSystem(walPath(4));

    wal->logCancel(1);
    wal->logModify(2, 20, 200);
    wal->logCancel(3);

    std::vector<LogEntry> entries;
    wal->recover([&](const LogEntry& e) {
        entries.push_back(e);
    });

    ASSERT_EQ(entries.size(), 3);
    EXPECT_EQ(entries[0].data.order_id, 1);
    EXPECT_EQ(entries[1].data.order_id, 2);
    EXPECT_EQ(entries[2].data.order_id, 3);
}


// test 5
// recover() — sequential replay correctness
TEST(WALTest, RecoverReplaysSequentially) {
    WALSystem *wal = new WALSystem(walPath(5));

    wal->logCancel(10);
    wal->logCancel(20);
    wal->logCancel(30);

    std::vector<OrderId> ids;
    wal->recover([&](const LogEntry& e) {
        ids.push_back(e.data.order_id);
    });

    ASSERT_EQ(ids.size(), 3);
    EXPECT_EQ(ids[0], 10);
    EXPECT_EQ(ids[1], 20);
    EXPECT_EQ(ids[2], 30);
}


// test 6
// recover() on missing WAL file
TEST(WALTest, RecoverOnMissingFileDoesNothing) {
    WALSystem *wal = new WALSystem("non_existent_wal_file");

    int count = 0;
    wal->recover([&](const LogEntry&) {
        count++;
    });

    EXPECT_EQ(count, 0);
}


// test 7
// ogTrade() — trade file correctness
TEST(WALTest, LogTradeWritesTradeFile) {
    std::string base = walPath(6);
    WALSystem *wal = new WALSystem(base);

    wal->logTrade(1, 2, 100, 50);

    std::ifstream file(base + ".trades");
    ASSERT_TRUE(file.is_open());

    std::string line;
    std::getline(file, line);

    EXPECT_EQ(line, "1,2,100,50");
}


// test 8
// WAL + Trade independence
TEST(WALTest, TradeLoggingDoesNotAffectWAL) {
    WALSystem *wal = new WALSystem(walPath(7));

    wal->logTrade(1, 2, 10, 5);
    wal->logCancel(42);

    std::vector<LogEntry> entries;
    wal->recover([&](const LogEntry& e) {
        entries.push_back(e);
    });

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].action, WalAction::CANCEL);
}


// test 9
/*
What this test PROVES

✔ ADD is logged before matching
✔ CANCEL is logged before removal
✔ MODIFY is logged before cancel + reinsert
✔ WAL order == input order
✔ WAL replay is deterministic
*/
TEST(MatchingEngineTest, WALInputOrderingAndCompleteness) {
    {
        MatchingEngine *engine = new MatchingEngine(28,global_bank);

        // ADD BUY
        engine->onNewOrder(
            1,          // order_id
            10,         // user_id
            Side::BUY,
            OrderType::LIMIT,
            10,         // price
            100,        // quantity
            1           // timestamp
        );

        // ADD SELL
        engine->onNewOrder(
            2,
            20,
            Side::SELL,
            OrderType::LIMIT,
            11,
            100,
            2
        );

        // CANCEL BUY
        engine->onCancelOrder(1);

        // MODIFY SELL
        engine->onModifyOrder(2, 12, 150);
    }

    // Read WAL
    std::vector<LogEntry> entries;
    WALSystem wal("engine_" + std::to_string(28));

    wal.recover([&](const LogEntry& entry) {
        entries.push_back(entry);
    });

    ASSERT_EQ(entries.size(), 5);

    EXPECT_EQ(entries[0].action, WalAction::ADD);
    EXPECT_EQ(entries[1].action, WalAction::ADD);
    EXPECT_EQ(entries[2].action, WalAction::CANCEL);
    EXPECT_EQ(entries[3].action, WalAction::MODIFY);
    EXPECT_EQ(entries[4].action, WalAction::CANCEL);

    EXPECT_EQ(entries[3].data.order_id, 2);
    EXPECT_EQ(entries[3].data.price, 12);
    EXPECT_EQ(entries[3].data.quantity, 150);
}


// test 10
/*
This validates:
	•	Determinism
	•	Correct WAL ordering
	•	Correct recovery logic
	•	No hidden state / randomness
	•	Replay safety
*/
TEST(MatchingEngineTest, DeterministicReplayFromWAL) {
    // -------- First run (generate WAL) --------
    {
        MatchingEngine *engine = new MatchingEngine(29,global_bank);

        // SELL 100 @ 10
        engine->onNewOrder(
            1, 10, Side::SELL, OrderType::LIMIT,
            10, 100, 1
        );

        // SELL 50 @ 11
        engine->onNewOrder(
            2, 10, Side::SELL, OrderType::LIMIT,
            11, 50, 2
        );

        // BUY 70 @ 10 (partial fill of s1)
        engine->onNewOrder(
            3, 20, Side::BUY, OrderType::LIMIT,
            10, 70, 3
        );

        // CANCEL SELL @ 11
        engine->onCancelOrder(2);
    }

    // -------- Recovery run --------
    MatchingEngine *recovered = new MatchingEngine(29,global_bank);

    // BUY side must be empty
    EXPECT_TRUE(recovered->order_book.buy_book.empty());

    // SELL side must have exactly one level (@10)
    ASSERT_EQ(recovered->order_book.sell_book.size(), 1);

    PriceLevel* level =
        recovered->order_book.sell_book.find(10);

    ASSERT_NE(level, nullptr);

    // Remaining qty must be 30
    EXPECT_EQ(level->aggregated_qty, 30);

    Order* order = level->head;
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->order_id, 1);
    EXPECT_EQ(order->remaining, 30);

    EXPECT_EQ(recovered->order_book.best_ask, level);
    EXPECT_TRUE(recovered->order_book.best_bid == nullptr);
}

///////////////////////////////////////////////////////////////////////////////////
// ------------------------------ NEW TESTS ---------------------------------------
///////////////////////////////////////////////////////////////////////////////////

TEST(MatchingEngineNewTest, SelfTradePreventionCancelsIncoming) {
    MatchingEngine *engine = new MatchingEngine(100,global_bank);

    engine->onNewOrder(1, 42, Side::SELL, OrderType::LIMIT, 10, 100, 1);
    engine->onNewOrder(2, 42, Side::BUY, OrderType::LIMIT, 10, 100, 2);

    // No match should occur
    PriceLevel* level = engine->order_book.sell_book.find(10);
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->aggregated_qty, 100);

    // Incoming order must be deallocated
    EXPECT_TRUE(engine->order_book.findOrder(2) == nullptr);
}

TEST(MatchingEngineNewTest, FilledOrderImmediateDeallocation) {
    MatchingEngine *engine = new MatchingEngine(101,global_bank);

    engine->onNewOrder(1,10,Side::SELL,OrderType::LIMIT,10,100,1);
    engine->onNewOrder(2,20,Side::BUY,OrderType::LIMIT,10,100,2);

    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);
    EXPECT_TRUE(engine->order_book.findOrder(2) == nullptr);
}

TEST(MatchingEngineNewTest, MarketOrderImmediateDeallocation) {
    MatchingEngine *engine = new MatchingEngine(102,global_bank);

    engine->onNewOrder(1,10,Side::BUY,OrderType::MARKET,0,100,1);

    EXPECT_TRUE(engine->order_book.buy_book.empty());
    EXPECT_TRUE(engine->order_book.findOrder(1) == nullptr);
}

TEST(MatchingEngineNewTest, WALRecoveryInternalOnNewOrderPath) {
    {
        MatchingEngine *engine = new MatchingEngine(103,global_bank);
        engine->onNewOrder(1,10,Side::SELL,OrderType::LIMIT,10,100,1);
        engine->onNewOrder(2,20,Side::BUY,OrderType::LIMIT,10,70,2);
    }

    MatchingEngine *recovered = new MatchingEngine(103,global_bank);

    ASSERT_EQ(recovered->order_book.sell_book.size(), 1);

    PriceLevel* level = recovered->order_book.sell_book.find(10);
    ASSERT_NE(level, nullptr);

    EXPECT_EQ(level->aggregated_qty, 30);
}


int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}