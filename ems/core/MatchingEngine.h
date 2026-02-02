#pragma once
#include <cstdint>

// Minimal matching engine stub.
// The EMS dispatcher calls this in strict per-symbol sequence order.
class MatchingEngine
{
public:
    void onNewOrder(
        uint32_t symbolId,
        int64_t price,
        uint32_t qty,
        uint8_t side,
        uint8_t type,
        uint32_t userId,
        uint64_t sequence) noexcept
    {
        (void)symbolId;
        (void)price;
        (void)qty;
        (void)side;
        (void)type;
        (void)userId;
        (void)sequence;
    }
};
