#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class EMSOrderTracker {
public:
    void trackNewOrder(const model::OrderRequest& request);
};

}