#ifndef EMS_PIPELINE_H
#define EMS_PIPELINE_H

#include "model/OrderRequest.h"
#include "model/EMSDecision.h"

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

::EMS::model::EMSDecision process(const ::EMS::model::OrderRequest& request);

private:
AuthService& auth_;
RiskManager& risk_;
MarketState& market_;
SymbolRouter& router_;
EMSOrderTracker& tracker_;
};

}

#endif