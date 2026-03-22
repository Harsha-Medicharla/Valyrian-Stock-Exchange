#include "pipeline/EMSPipeline.h"
#include "model/RejectReason.h"

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
                         EMSOrderTracker& tracker,
                         Settlement::SettlementModule& settlement)
    : auth_(auth), risk_(risk), market_(market), router_(router), tracker_(tracker), settlement_(settlement) {}

model::EMSDecision EMSPipeline::process(const model::OrderRequest& request) {
    model::EMSDecision decision;
    decision.accepted = true;
    decision.reason = model::RejectReason::NONE;
    decision.original_request = request;
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

    if (!settlement_.reserveMargin(request.user_id, request.symbol, request.side, request.price, request.quantity)) {
        decision.accepted = false;
        decision.reason = model::RejectReason::INSUFFICIENT_FUNDS; 
        return decision;
    }
    tracker_.trackNewOrder(request);
    return decision;
}
}