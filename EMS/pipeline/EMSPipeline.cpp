#include "EMS/pipeline/EMSPipeline.h"
#include "EMS/model/OrderRequest.h"
#include "EMS/model/EMSDecision.h"
#include "EMS/model/RejectReason.h"
#include "EMS/auth/AuthService.h"
#include "EMS/risk/RiskManager.h"
#include "EMS/market/MarketState.h"
#include "EMS/routing/SymbolRouter.h"
#include "EMS/tracker/EMSOrderTracker.h"
#include "Settlement/core/Settlement.h"

namespace EMS {

EMSPipeline::EMSPipeline(AuthService& auth,
                         RiskManager& risk,
                         MarketState& market,
                         SymbolRouter& router,
                         EMSOrderTracker& tracker,
                         SettlementCore::Settlement& settlement)
    : auth_(auth), risk_(risk), market_(market), router_(router), tracker_(tracker), settlement_(settlement) {}

EMS::model::EMSDecision EMSPipeline::process(const EMS::model::OrderRequest& request) {
    EMS::model::EMSDecision decision;
    decision.accepted = true;
    decision.reason = EMS::model::RejectReason::NONE;
    decision.original_request = request;

    // 1. Auth Check
    if (!auth_.isAuthorized(request.user_id)) {
        decision.accepted = false;
        decision.reason = EMS::model::RejectReason::AUTH_FAILED;
        return decision;
    } 
    
    // 2. Market State Check
    if (!market_.isSymbolOpen(request.symbol)) {
        decision.accepted = false;
        decision.reason = EMS::model::RejectReason::MARKET_CLOSED;
        return decision;
    }
    
    // 3. Risk Check
    if (!risk_.passesRisk(request)) {
        decision.accepted = false;
        decision.reason = EMS::model::RejectReason::RISK_EXCEEDED;
        return decision;
    }

    // 4. Settlement/Margin Check (Now matching the uint64_t Symbol type)
    if (!settlement_.reserveMargin(request.user_id, request.symbol, static_cast<uint8_t>(request.side), request.price, request.quantity)) {
        decision.accepted = false;
        decision.reason = EMS::model::RejectReason::INSUFFICIENT_FUNDS; 
        return decision;
    }

    // 5. Tracking
    tracker_.trackNewOrder(request);
    
    return decision;
}

} // namespace EMS