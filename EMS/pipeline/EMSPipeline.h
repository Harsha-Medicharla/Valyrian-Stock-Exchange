#pragma once
#include "../model/OrderRequest.h"
#include "../model/EMSDecision.h"
#include "Settlement/core/Settlement.h"

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
                EMSOrderTracker& tracker,
                SettlementCore::Settlement& settlement);

    model::EMSDecision process(const model::OrderRequest& request);

private:
    AuthService& auth_;
    RiskManager& risk_;
    MarketState& market_;
    SymbolRouter& router_;
    EMSOrderTracker& tracker_;
    SettlementCore::Settlement& settlement_;
};
}