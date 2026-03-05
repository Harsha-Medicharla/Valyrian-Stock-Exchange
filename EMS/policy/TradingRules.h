#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class TradingRules {
public:
    bool isValidTickSize(const model::OrderRequest& request) const {
        return true; // e.g., price is a multiple of $0.01
    }
    
    bool isValidLotSize(const model::OrderRequest& request) const {
        return true; // e.g., quantity is a multiple of 100
    }
};

}