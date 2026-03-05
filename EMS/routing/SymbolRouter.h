#pragma once
#include <cstdint>
#include "../model/OrderRequest.h"

namespace EMS {

class SymbolRouter {
public:
    uint16_t route(const model::OrderRequest& request);
};

}