#include <gtest/gtest.h>
#include "pipeline/EMSPipeline.h"
#include "auth/AuthService.h"
#include "risk/RiskManager.h"
#include "market/MarketState.h"
#include "routing/SymbolRouter.h"
#include "tracker/EMSOrderTracker.h"
#include "model/OrderRequest.h"
#include "model/EMSDecision.h"

using namespace EMS;
using namespace EMS::model;

class EMSPipelineTest : public ::testing::Test {
protected:
    // 1. Initialize the services
    AuthService auth;
    RiskManager risk;
    MarketState market;
    SymbolRouter router;
    EMSOrderTracker tracker;

    // 2. Inject them into the pipeline
    EMSPipeline pipeline{auth, risk, market, router, tracker};
};

TEST_F(EMSPipelineTest, ValidOrderIsAccepted) {
    OrderRequest req;
    req.user_id = 1;
    req.symbol = 1;
    req.quantity = 100;

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    OrderRequest req;
    req.user_id = 0; // Trigger the Auth failure
    req.symbol = 1;
    req.quantity = 100;

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    OrderRequest req;
    req.user_id = 1;
    req.symbol = 0; // Trigger the Market failure
    req.quantity = 100;

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

TEST_F(EMSPipelineTest, RejectsFatFingerRisk) {
    OrderRequest req;
    req.user_id = 1;
    req.symbol = 1;
    req.quantity = 1000001; // Trigger the Risk failure (> 1,000,000)

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}