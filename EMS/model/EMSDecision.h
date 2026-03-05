#pragma once
#include "OrderRequest.h" 

namespace EMS {
namespace model {

enum class RejectReason {
    NONE,
    AUTH_FAILED,
    MARKET_CLOSED,
    RISK_EXCEEDED
};

struct EMSDecision {
    bool accepted;
    RejectReason reason;
    OrderRequest original_request; 
};

}
}