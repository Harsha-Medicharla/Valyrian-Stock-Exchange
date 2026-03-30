#pragma once
#include <cstdint>

// Ring-buffer slot lifecycle for the dispatcher (see IngressWorker claim-before-validate).
enum class OrderSlotState : uint8_t
{
    Pending = 0,
    Valid = 1,
    Rejected = 2,
};

struct alignas(64) OrderSlot
{
    uint64_t sequence;

    uint64_t order_id;
    uint64_t timestamp;

    uint32_t user_id;
    uint32_t symbol_id;

    int64_t price;
    uint32_t qty;

    uint8_t side;
    uint8_t type;

    OrderSlotState state = OrderSlotState::Pending;
};
