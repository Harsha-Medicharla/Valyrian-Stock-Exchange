#pragma once
#include "../model/OrderRequest.h"

namespace EMS {

class EMSOrderTracker {
public:
    void record(const model::OrderRequest&);
};

}
