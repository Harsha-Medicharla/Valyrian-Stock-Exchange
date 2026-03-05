#include "pipeline/EMSPipeline.h"
#include "auth/AuthService.h"
#include "risk/RiskManager.h"
#include "market/MarketState.h"     
#include "routing/SymbolRouter.h"
#include "tracker/EMSOrderTracker.h"

namespace EMS {

EMSPipeline::EMSPipeline(AuthService& auth,
                         RiskManager& risk,
                         MarketState& market,
                         SymbolRouter& router,
                         EMSOrderTracker& tracker)
    : auth_(auth), risk_(risk), market_(market), router_(router), tracker_(tracker) {}

model::EMSDecision EMSPipeline::process(const model::OrderRequest& request) {
    model::EMSDecision decision;
    decision.accepted = true;
    decision.reason = model::RejectReason::NONE;

    if (!auth_.isAuthorized(request.user_id)) {
        decision.accepted = false;
        decision.reason = model::RejectReason::AUTH_FAILED;
        return decision;
    } 
    
    if (!market_.isSymbolOpen(request.symbol)) {
        decision.accepted = false;
        decision.reason = model::RejectReason::MARKET_CLOSED;
        return decision;
    }
    
    if (!risk_.passesRisk(request)) {
        decision.accepted = false;
        decision.reason = model::RejectReason::RISK_EXCEEDED;
        return decision;
    }
    tracker_.trackNewOrder(request);
    decision.original_request = request;
    return decision;
}

}