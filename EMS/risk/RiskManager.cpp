#include "risk/RiskManager.h"
#include "core/EMSConfig.h"

namespace EMS {

bool RiskManager::passesRisk(const model::OrderRequest& request) const {
    return request.quantity <= EMSConfig::FAT_FINGER_LIMIT;
}

}