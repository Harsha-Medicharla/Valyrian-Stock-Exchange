#include "EMSPipeline.h"

namespace EMS {

EMSPipeline::EMSPipeline(AuthService& auth,
RiskManager& risk,
MarketState& market,
SymbolRouter& router,
EMSOrderTracker& tracker)
: auth_(auth), risk_(risk), market_(market), router_(router), tracker_(tracker) {}

::EMS::model::EMSDecision EMSPipeline::process(const ::EMS::model::OrderRequest& request) {
::EMS::model::EMSDecision decision;
decision.accepted = true;
decision.reason = ::EMS::model::RejectReason::NONE;

if (request.user_id == 0) {
decision.accepted = false;
decision.reason = ::EMS::model::RejectReason::AUTH_FAILED;
} else if (request.symbol == 0) {
decision.accepted = false;
decision.reason = ::EMS::model::RejectReason::MARKET_CLOSED;
} else if (request.quantity > 1000000) {
decision.accepted = false;
decision.reason = ::EMS::model::RejectReason::RISK_EXCEEDED;
}

return decision;
}

}