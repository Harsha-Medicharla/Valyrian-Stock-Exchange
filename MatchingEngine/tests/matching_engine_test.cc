#include <gtest/gtest.h>
#include "MatchingEngine.h"

TEST(MatchingEngineTest, EmptyBookOnStartup) {
    // final test of current implementation
    // -------- First run (generate WAL) --------
    // {
    //     MatchingEngine *engine = new MatchingEngine(666);

    //     // SELL 100 @ 10
    //     Order* s1 = engine->order_book.requestAllocationOfOrder();
    //     *s1 = {1, 10, Side::SELL, OrderType::LIMIT, 10, 100, 100, 1, OrderState::NEW, nullptr, nullptr};
    //     engine->onNewOrder(s1);

    //     // SELL 50 @ 11
    //     Order* s2 = engine->order_book.requestAllocationOfOrder();
    //     *s2 = {2, 10, Side::SELL, OrderType::LIMIT, 11, 50, 50, 2, OrderState::NEW, nullptr, nullptr};
    //     engine->onNewOrder(s2);

    //     // BUY 70 @ 10 (partial fill of s1)
    //     Order* b1 = engine->order_book.requestAllocationOfOrder();
    //     *b1 = {3, 20, Side::BUY, OrderType::LIMIT, 10, 70, 70, 3, OrderState::NEW, nullptr, nullptr};
    //     engine->onNewOrder(b1);

    //     // CANCEL SELL @ 11
    //     engine->onCancelOrder(2);
    // }

    // // -------- Recovery run --------
    // MatchingEngine *recovered = new MatchingEngine(666);

    // // -------- Assertions --------

    // // BUY side must be empty
    // EXPECT_TRUE(recovered->order_book.buy_book.empty());

    // // SELL side must have exactly one level (@10)
    // ASSERT_EQ(recovered->order_book.sell_book.size(), 1);

    // auto it = recovered->order_book.sell_book.find(10);
    // ASSERT_NE(it, nullptr);

    // PriceLevel* level = it;

    // // Remaining qty must be 30
    // EXPECT_EQ(level->aggregated_qty, 30);

    // Order* order = level->head;
    // ASSERT_NE(order, nullptr);

    // EXPECT_EQ(order->order_id, 1);
    // EXPECT_EQ(order->remaining, 30);

    // // Best ask must be correct
    // EXPECT_EQ(recovered->order_book.best_ask, level);
    // EXPECT_TRUE(recovered->order_book.best_bid == nullptr);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
