#include <gtest/gtest.h>

#include "EMS/pipeline/EMSPipeline.h"
#include "EMS/auth/AuthService.h"
#include "EMS/risk/RiskManager.h"
#include "EMS/market/MarketState.h"
#include "EMS/routing/SymbolRouter.h"
#include "EMS/tracker/EMSOrderTracker.h"

using namespace EMS;
using namespace EMS::model;

class EMSPipelineIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<AuthService> auth;
    std::unique_ptr<RiskManager> risk;
    std::unique_ptr<MarketState> market;
    std::unique_ptr<SymbolRouter> router;
    std::unique_ptr<EMSOrderTracker> tracker;

    std::unique_ptr<EMSPipeline> pipeline;

    void SetUp() override {
        auth = std::make_unique<AuthService>();
        risk = std::make_unique<RiskManager>();
        market = std::make_unique<MarketState>();
        router = std::make_unique<SymbolRouter>();
        tracker = std::make_unique<EMSOrderTracker>();

        pipeline = std::make_unique<EMSPipeline>(
            *auth,
            *risk,
            *market,
            *router,
            *tracker
        );
    }

    OrderRequest makeBaseOrder() {
        OrderRequest req{};
        req.order_id = 1;
        req.user_id = 42;
        req.symbol = 1; // assuming Symbol is numeric ID
        req.side = Side::BUY;
        req.type = OrderType::LIMIT;
        req.price = 100;
        req.quantity = 10;
        req.wall_time_ns = 123456789;
        req.event_seq = 1;
        return req;
    }
};

TEST_F(EMSPipelineIntegrationTest, AcceptsValidOrder) {
    auto order = makeBaseOrder();

    auto decision = pipeline->process(order);

    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineIntegrationTest, RejectsZeroQuantity) {
    auto order = makeBaseOrder();
    order.quantity = 0;

    auto decision = pipeline->process(order);

    EXPECT_FALSE(decision.accepted);
    EXPECT_NE(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineIntegrationTest, RejectsZeroPriceForLimit) {
    auto order = makeBaseOrder();
    order.price = 0;

    auto decision = pipeline->process(order);

    EXPECT_FALSE(decision.accepted);
    EXPECT_NE(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineIntegrationTest, HandlesLargeQuantityOrder) {
    auto order = makeBaseOrder();
    order.quantity = 1'000'000;

    auto decision = pipeline->process(order);

    // Depending on your risk config, adjust expectation:
    EXPECT_TRUE(decision.accepted);
}

TEST_F(EMSPipelineIntegrationTest, DifferentEventSequences) {
    auto order1 = makeBaseOrder();
    auto order2 = makeBaseOrder();
    order2.event_seq = 2;

    auto d1 = pipeline->process(order1);
    auto d2 = pipeline->process(order2);

    EXPECT_TRUE(d1.accepted);
    EXPECT_TRUE(d2.accepted);
}

TEST_F(EMSPipelineIntegrationTest, DifferentUsers) {
    auto order1 = makeBaseOrder();
    auto order2 = makeBaseOrder();
    order2.user_id = 999;

    auto d1 = pipeline->process(order1);
    auto d2 = pipeline->process(order2);

    EXPECT_TRUE(d1.accepted);
    EXPECT_TRUE(d2.accepted);
}

TEST_F(EMSPipelineIntegrationTest, MarketOrderWithZeroPrice) {
    auto order = makeBaseOrder();
    order.type = OrderType::MARKET;
    order.price = 0;

    auto decision = pipeline->process(order);

    // Market orders often allow zero price — adjust if needed
    EXPECT_TRUE(decision.accepted);
}

TEST_F(EMSPipelineIntegrationTest, MultipleSequentialOrders) {
    for (int i = 0; i < 100; ++i) {
        auto order = makeBaseOrder();
        order.order_id = i + 1;
        order.event_seq = i + 1;

        auto decision = pipeline->process(order);

        EXPECT_TRUE(decision.accepted);
    }
}
