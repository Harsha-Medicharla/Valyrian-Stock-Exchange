#pragma once
#include "../model/EMSDecision.h"

namespace EMS {

class EgressPort {
public:
    virtual ~EgressPort() = default;
    virtual void forward(const model::EMSDecision& decision) = 0;
};

}