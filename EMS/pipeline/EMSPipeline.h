#pragma once

#include "../model/OrderRequest.h"
#include "../model/EMSDecision.h"

namespace EMS {
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

    model::EMSDecision process(const model::OrderRequest& request);

private:
    // Core services injected via constructor
    AuthService& auth_;
    RiskManager& risk_;
    MarketState& market_;
    SymbolRouter& router_;
    EMSOrderTracker& tracker_;
};

}