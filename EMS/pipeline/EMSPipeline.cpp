#include "model/OrderRequest.h"
#include "EMSPipeline.h"
#include "model/EMSDecision.h"
#include "model/RejectReason.h"

using EMS::model::OrderRequest;
using EMS::model::EMSDecision;
using EMS::model::RejectReason;


EMSPipeline::EMSPipeline(AuthService& auth,
                         RiskManager& risk,
                         MarketState& market,
                         SymbolRouter& router,
                         EMSOrderTracker& tracker)
    : auth_(auth),
      risk_(risk),
      market_(market),
      router_(router),
      tracker_(tracker) {}

EMSDecision EMSPipeline::process(const OrderRequest& request) {


    if (!auth_.isAuthenticated(request.user_id)) {
        return EMSDecision::reject(RejectReason::AUTH_FAILED);
    }


    if (!market_.isOpen(request.symbol)) {
        return EMSDecision::reject(RejectReason::MARKET_CLOSED);
    }

    RejectReason reason;
    if (!risk_.check(request, reason)) {
        return EMSDecision::reject(reason);
    }

    size_t route = router_.route(request.symbol);

    tracker_.markValidated(request.order_id);

    return EMSDecision::accept(route);
}
