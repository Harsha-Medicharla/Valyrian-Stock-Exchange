#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class IngressPort {
public:
    virtual ~IngressPort() = default;
    virtual void receive(const model::OrderRequest& request) = 0;
};

}