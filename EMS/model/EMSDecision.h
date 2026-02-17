#pragma once

#include "RejectReason.h"
#include <cstddef>

namespace EMS {
namespace model {

struct EMSDecision {
    bool accepted;
    RejectReason reason;
    size_t route_index;

    static EMSDecision Accept(size_t route) {
        return {true, RejectReason::NONE, route};
    }

    static EMSDecision Reject(RejectReason r) {
        return {false, r, 0};
    }
};

}
}
