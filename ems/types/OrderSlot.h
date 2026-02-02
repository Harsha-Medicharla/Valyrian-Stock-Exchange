#pragma once
#include <cstdint>

struct alignas(64) OrderSlot
{
    uint64_t sequence;

    uint32_t user_id;
    uint32_t symbol_id;

    int64_t price;
    uint32_t qty;

    uint8_t side;
    uint8_t type;
};
