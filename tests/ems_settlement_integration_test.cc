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
    // Members are fresh for every single TEST_F execution
    AuthService                 auth;
    RiskManager                 risk;
    MarketState                 market;
    SymbolRouter                router;
    EMSOrderTracker             tracker;
    SettlementCore::Settlement  bank;
    
    std::unique_ptr<EMSPipeline> pipeline;

    void SetUp() override {
        // Open symbol 0 for trading
        market.openSymbol(0);
        
        // Seed User 1 with cash (100 million cents/units)
        bank.adminDeposit(1, 100000000); 

        // Initialize the pipeline with dependencies
        pipeline = std::make_unique<EMSPipeline>(auth, risk, market, router, tracker, bank);
    }

    // TearDown is handled automatically; the Settlement destructor 
    // will join the background thread safely.

    OrderRequest createBaseOrder() {
        OrderRequest req;
        req.user_id = 1;      
        req.symbol = 0;       
        req.side = Side::BUY; 
        req.price = 100;      
        req.quantity = 100;   
        return req;
    }
};

// --- 1. SUCCESS PATHS ---

TEST_F(EMSPipelineTest, ValidBuyOrderIsAccepted) {
    auto req = createBaseOrder();
    auto decision = pipeline->process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

TEST_F(EMSPipelineTest, AcceptsSellWithSufficientShares) {
    // Give User 1 some shares of Symbol 0
    bank.adminDepositShares(1, 0, 1000); 
    
    auto req = createBaseOrder();
    req.side = Side::SELL;
    req.quantity = 500;
    
    auto decision = pipeline->process(req);
    
    EXPECT_TRUE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::NONE);
}

// --- 2. MARGIN & INVENTORY FAILURES ---

TEST_F(EMSPipelineTest, RejectsInsufficientFunds) {
    auto req = createBaseOrder();
    
    // Total cost exceeds the 100M deposit
    req.price = 10000; 
    req.quantity = 1000000;

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::INSUFFICIENT_FUNDS);
}

TEST_F(EMSPipelineTest, RejectsNakedShortSell) {
    auto req = createBaseOrder();
    req.side = Side::SELL;
    req.quantity = 1; // User has 0 shares of Symbol 0 initially
    
    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::INSUFFICIENT_SHARES); 
}

// --- 3. AUTHENTICATION FAILURES ---

TEST_F(EMSPipelineTest, RejectsUnauthorizedUser) {
    auto req = createBaseOrder();
    req.user_id = 0; // Blacklisted ID

    auto decision = pipeline->process(req);
    
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}

// --- 4. MARKET STATE FAILURES ---

TEST_F(EMSPipelineTest, RejectsClosedMarketSymbol) {
    auto req = createBaseOrder();
    req.symbol = 99; // Symbol 99 was never opened

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

// --- 6. SHORT-CIRCUIT & LOGIC ---

TEST_F(EMSPipelineTest, AuthFailsBeforeMarginCheck) {
    auto req = createBaseOrder();
    req.user_id = 0;      // Fails Gate 1
    req.price = 1e12;     // Would fail Gate 4 (Margin)

    auto decision = pipeline->process(req);
    
    // Logic should stop at Auth failure
    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, RejectReason::AUTH_FAILED);
}