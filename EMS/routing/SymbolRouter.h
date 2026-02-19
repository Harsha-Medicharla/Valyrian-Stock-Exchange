#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class SymbolRouter {
public:
    void route(const model::OrderRequest& request);
};

}