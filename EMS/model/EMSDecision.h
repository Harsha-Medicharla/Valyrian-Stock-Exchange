#pragma once
#include "OrderRequest.h" 
#include "RejectReason.h"
namespace EMS {
namespace model {


struct EMSDecision {
    bool accepted;
    RejectReason reason;
    OrderRequest original_request; 
};

}
}