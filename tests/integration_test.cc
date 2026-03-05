#include <gtest/gtest.h>
#include "pipeline/EMSPipeline.h"
#include "auth/AuthService.h"
#include "risk/RiskManager.h"
#include "market/MarketState.h"
#include "routing/SymbolRouter.h"
#include "tracker/EMSOrderTracker.h"
#include "model/OrderRequest.h"
#include "model/EMSDecision.h"
#include "core/EMSConfig.h" 

using namespace EMS;
using namespace EMS::model;

class EMSPipelineTest : public ::testing::Test {
protected:
    AuthService auth;
    RiskManager risk;
    MarketState market;
    SymbolRouter router;
    EMSOrderTracker tracker;

    EMSPipeline pipeline{auth, risk, market, router, tracker};

    // Helper to generate a clean baseline order
    OrderRequest createValidOrder() {
        OrderRequest req;
        req.user_id = 1;      // Valid user
        req.symbol = 1;       // Open market
        req.quantity = 100;   // Safe quantity
        return req;
    }
};

// --- 1. HAPPY PATHS ---

TEST_F(EMSPipelineTest, ValidOrderIsAccepted) {
    OrderRequest req = createValidOrder();
    EMSDecision decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

// --- 2. AUTHENTICATION FAILURES ---

TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    OrderRequest req = createValidOrder();
    req.user_id = 0; // Invalid user

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

// --- 3. MARKET STATE FAILURES ---

TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    OrderRequest req = createValidOrder();
    req.symbol = 0; // Closed market

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

// --- 4. RISK BOUNDARY TESTING ---

TEST_F(EMSPipelineTest, AcceptsOrderExactlyAtRiskLimit) {
    OrderRequest req = createValidOrder();
    // Test the exact boundary: 1,000,000 should PASS
    req.quantity = EMSConfig::FAT_FINGER_LIMIT; 

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineTest, RejectsOrderJustOverRiskLimit) {
    OrderRequest req = createValidOrder();
    // Test the exact boundary + 1: 1,000,001 should FAIL
    req.quantity = EMSConfig::FAT_FINGER_LIMIT + 1; 

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

// --- 5. ORDER OF OPERATIONS (SHORT-CIRCUIT CHECKS) ---

TEST_F(EMSPipelineTest, AuthFailsBeforeMarketCheck) {
    OrderRequest req = createValidOrder();
    req.user_id = 0;  // Should fail Auth
    req.symbol = 0;   // Should fail Market

    EMSDecision decision = pipeline.process(req);
    
    // Auth is checked first, so the reason MUST be AUTH_FAILED, not MARKET_CLOSED
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

TEST_F(EMSPipelineTest, AuthFailsBeforeRiskCheck) {
    OrderRequest req = createValidOrder();
    req.user_id = 0;      // Should fail Auth
    req.quantity = 99999999; // Should fail Risk

    EMSDecision decision = pipeline.process(req);
    
    // Must short-circuit at Auth
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

TEST_F(EMSPipelineTest, MarketFailsBeforeRiskCheck) {
    OrderRequest req = createValidOrder();
    req.symbol = 0;       // Should fail Market
    req.quantity = 99999999; // Should fail Risk

    EMSDecision decision = pipeline.process(req);
    
    // Must short-circuit at Market
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

// --- 6. EXTREME EDGE CASES ---

TEST_F(EMSPipelineTest, HandlesMaxUint64Quantity) {
    OrderRequest req = createValidOrder();
    req.quantity = std::numeric_limits<uint64_t>::max(); 

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

TEST_F(EMSPipelineTest, HandlesZeroQuantity) {
    OrderRequest req = createValidOrder();
    req.quantity = 0; // A 0 quantity is technically safe from fat-finger, but usually invalid. 
                      // For now, our risk manager just checks if it's <= 1,000,000.
                      // So it should pass the EMS pipeline (the ME will likely drop it later).

    EMSDecision decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}