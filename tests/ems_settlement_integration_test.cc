#include <gtest/gtest.h>
#include <memory>
#include "EMS/pipeline/EMSPipeline.h"
#include "EMS/auth/AuthService.h"
#include "EMS/risk/RiskManager.h"
#include "EMS/market/MarketState.h"
#include "EMS/routing/SymbolRouter.h"
#include "EMS/tracker/EMSOrderTracker.h"
#include "EMS/model/OrderRequest.h"
#include "EMS/model/EMSDecision.h"
#include "EMS/core/EMSConfig.h" 
#include "Settlement/core/Settlement.h"

using namespace EMS;
using namespace EMS::model;

class EMSPipelineTest : public ::testing::Test {
protected:
    // 1. Members instead of global statics. 
    // This guarantees a fresh instance is created for EVERY individual test.
    AuthService                 auth;
    RiskManager                 risk;
    MarketState                 market;
    SymbolRouter                router;
    EMSOrderTracker             tracker;
    SettlementCore::Settlement  bank;
    
    std::unique_ptr<EMSPipeline> pipeline;

    void SetUp() override {
        // 2. Baseline state setup
        market.openSymbol(0);
        
        // Seed User 1 with enough cash for standard tests (100 million)
        bank.adminDeposit(1, 100000000); 

        // 3. Initialize the pipeline with the fresh dependencies
        pipeline = std::make_unique<EMSPipeline>(auth, risk, market, router, tracker, bank);
    }

    // Helper method to generate a standard, valid order
    OrderRequest createBaseOrder() {
        OrderRequest req;
        req.user_id = 1;      
        req.symbol = 0;       
        req.side = Side::BUY; 
        req.price = 100;      // Using int64_t representation (e.g., Cents)
        req.quantity = 100;   
        return req;
    }
};

// --- 1. SUCCESS PATH ---
TEST_F(EMSPipelineTest, ValidOrderIsAccepted) {
    auto req = createBaseOrder();
    auto decision = pipeline->process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

// --- 2. MARGIN FAILURES ---
TEST_F(EMSPipelineTest, RejectsInsufficientFunds) {
    auto req = createBaseOrder();
    
    // Total cost = 1 Billion. User only has 100 Million.
    req.price = 1000; 
    req.quantity = 1000000;

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::INSUFFICIENT_FUNDS);
}

// --- 3. AUTHENTICATION FAILURES ---
TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    auto req = createBaseOrder();
    req.user_id = 0; // Assuming ID 0 is invalid/blacklisted in AuthService

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

// --- 4. MARKET STATE FAILURES ---
TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    auto req = createBaseOrder();
    req.symbol = 99; // Never opened in SetUp()

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::MARKET_CLOSED);
}

// --- 5. RISK BOUNDARY TESTING ---
TEST_F(EMSPipelineTest, AcceptsOrderExactlyAtRiskLimit) {
    auto req = createBaseOrder();
    req.quantity = EMSConfig::FAT_FINGER_LIMIT; 

    auto decision = pipeline->process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineTest, RejectsOrderJustOverRiskLimit) {
    auto req = createBaseOrder();
    req.quantity = EMSConfig::FAT_FINGER_LIMIT + 1; 

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::RISK_EXCEEDED);
}

// --- 6. SHORT-CIRCUIT LOGIC ---
TEST_F(EMSPipelineTest, AuthFailsBeforeMarginCheck) {
    auto req = createBaseOrder();
    req.user_id = 0;      // Fails Gate 1 (Auth)
    req.price = 1000000;  // Would also fail Gate 4 (Settlement margin)
    req.quantity = 1000000;

    auto decision = pipeline->process(req);
    
    // It should reject immediately for AUTH_FAILED, never reaching INSUFFICIENT_FUNDS
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}