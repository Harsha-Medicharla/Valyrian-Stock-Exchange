#include "risk/RiskManager.h"

namespace EMS {

bool RiskManager::passesRisk(const model::OrderRequest& request) const {
    return request.quantity <= 1000000;
}

}