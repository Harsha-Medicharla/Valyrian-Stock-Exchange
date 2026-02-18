#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class RiskManager {
public:
    bool passesRisk(const model::OrderRequest&) const;
};

}
