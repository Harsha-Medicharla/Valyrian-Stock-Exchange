#pragma once
#include <cstdint>

enum class OrderSlotState : uint8_t
{
    Pending = 0,
    Valid = 1,
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
    uint8_t cancel_flag;
    uint8_t modify_flag;
};

static_assert(sizeof(OrderSlot) == 64, "OrderSlot must remain 64 bytes");
