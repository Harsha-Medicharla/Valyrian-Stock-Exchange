#include <gtest/gtest.h>
#include <memory>
#include "pipeline/EMSPipeline.h"
#include "auth/AuthService.h"
#include "risk/RiskManager.h"
#include "market/MarketState.h"
#include "routing/SymbolRouter.h"
#include "tracker/EMSOrderTracker.h"
#include "model/OrderRequest.h"
#include "model/EMSDecision.h"
#include "core/EMSConfig.h" 
#include "SettlementModule.h"

using namespace EMS;
using namespace EMS::model;

// Static globals ensure memory alignment and persist across the test process
// avoiding the stack-overflow "Illegal Instruction" issues.
static AuthService    g_auth;
static RiskManager    g_risk;
static MarketState    g_market;
static SymbolRouter   g_router;
static EMSOrderTracker g_tracker;
static Settlement::SettlementModule g_bank;

class EMSPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset Market
        g_market.openSymbol(0);

        // HARD RESET the bank for User 1
        // We give them $1 Billion to ensure the Settlement gate 
        // stays open during high-quantity Risk tests.
        g_bank.users[1].available_cash = 1'000'000'000.0;
        g_bank.users[1].available_stocks[0] = 10'000'000; 
    }

    OrderRequest createBaseOrder() {
        OrderRequest req;
        req.user_id = 1;      
        req.symbol = 0;       
        req.side = Side::BUY; 
        req.price = 100.0;    
        req.quantity = 100;   
        return req;
    }
};

// --- 1. SUCCESS PATH ---
TEST_F(EMSPipelineTest, ValidOrderIsAccepted) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    auto decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

// --- 2. MARGIN FAILURES ---
TEST_F(EMSPipelineTest, RejectsInsufficientFunds) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    // Total cost $2 Billion (User only has $1 Billion)
    req.price = 2000.0; 
    req.quantity = 1'000'000;

    auto decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::INSUFFICIENT_FUNDS);
}

// --- 3. AUTHENTICATION FAILURES ---
TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.user_id = 0; // Blacklisted

    auto decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

// --- 4. MARKET STATE FAILURES ---
TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.symbol = 99; // Closed

    auto decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

// --- 5. RISK BOUNDARY TESTING ---
TEST_F(EMSPipelineTest, AcceptsOrderExactlyAtRiskLimit) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.quantity = EMSConfig::FAT_FINGER_LIMIT; 
    // Price at 100.0 means cost is $100M. User has $1B. This should PASS.

    auto decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineTest, RejectsOrderJustOverRiskLimit) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.quantity = EMSConfig::FAT_FINGER_LIMIT + 1; 

    auto decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

// --- 6. SHORT-CIRCUIT LOGIC ---
TEST_F(EMSPipelineTest, AuthFailsBeforeMarginCheck) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.user_id = 0;      // Fail Gate 1
    req.price = 1e12;     // Would fail Gate 4

    auto decision = pipeline.process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}