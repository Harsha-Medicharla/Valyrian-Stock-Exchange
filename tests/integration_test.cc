#include <gtest/gtest.h>
#include "pipeline/EMSPipeline.h"
#include "auth/AuthService.h"
#include "risk/RiskManager.h"
#include "market/MarketState.h"
#include "routing/SymbolRouter.h"
#include "tracker/EMSOrderTracker.h"
#include "SettlementModule.h"

using namespace EMS;
using namespace EMS::model;

// --- STATIC MEMORY POOL ---
// This prevents stack overflows and VTable misalignment (the SIGILL killers)
static AuthService    g_auth;
static RiskManager    g_risk;
static MarketState    g_market;
static SymbolRouter   g_router;
static EMSOrderTracker g_tracker;
static Settlement::SettlementModule g_bank;

class EMSPipelineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset state before every single test
        g_market.openSymbol(0);
        g_bank.users[1].available_cash = 10000000.0;
        g_bank.users[1].available_stocks[0] = 10000;
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

// 1. SUCCESS PATH
TEST_F(EMSPipelineTest, ValidOrderIsAccepted) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    auto decision = pipeline.process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

// 2. FUNDING GATE
TEST_F(EMSPipelineTest, RejectsInsufficientFunds) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.price = 10000001.0; // Total > 10M balance

    auto decision = pipeline.process(req);
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::INSUFFICIENT_FUNDS);
}

// 3. AUTH GATE
TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.user_id = 0; // Blacklisted ID

    auto decision = pipeline.process(req);
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

// 4. MARKET STATE GATE
TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.symbol = 99; // Never opened in SetUp

    auto decision = pipeline.process(req);
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

// 5. RISK GATE (FAT FINGER)
TEST_F(EMSPipelineTest, RejectsOrderAboveRiskLimit) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.quantity = 2000000; // Well above the 1M limit

    auto decision = pipeline.process(req);
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

// 6. PIPELINE PRIORITY (Auth should trigger before Risk)
TEST_F(EMSPipelineTest, AuthFailsBeforeOtherChecks) {
    EMSPipeline pipeline(g_auth, g_risk, g_market, g_router, g_tracker, g_bank);
    auto req = createBaseOrder();
    req.user_id = 0;   // Should fail Auth
    req.quantity = 5000000; // Should also fail Risk

    auto decision = pipeline.process(req);
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}