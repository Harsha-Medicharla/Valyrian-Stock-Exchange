#pragma once
#include <cstdint>
#include "../types/OrderSlot.h"

// Minimal matching engine stub.
// The EMS dispatcher calls this in strict per-symbol sequence order.
class MatchingEngine
{
public:
    inline void onNewOrder(const OrderSlot &order) noexcept
    {
        (void)order;
    }
};
