#pragma once
#include "EMSDecision.h"
#include <cstdint>

namespace EMS {
namespace model {

enum class ResponseType : uint8_t {
    EMS_DECISION,
    EXECUTION_REPORT
};

struct ClientResponse {
    ResponseType type;
    EMSDecision decision; 
    
    // Default constructor
    ClientResponse() = default;
    
    // Construct from an EMSDecision
    explicit ClientResponse(const EMSDecision& d) 
        : type(ResponseType::EMS_DECISION), decision(d) {}
};

}
}
