#pragma once

#include "model/OrderRequest.h"
#include "model/EMSDecision.h"

class AuthService;
class RiskManager;
class MarketState;
class SymbolRouter;
class EMSOrderTracker;

class EMSPipeline {
public:
    EMSPipeline(AuthService& auth,
                RiskManager& risk,
                MarketState& market,
                SymbolRouter& router,
                EMSOrderTracker& tracker);

    EMSDecision process(const OrderRequest& request);

private:
    AuthService& auth_;
    RiskManager& risk_;
    MarketState& market_;
    SymbolRouter& router_;
    EMSOrderTracker& tracker_;
};
