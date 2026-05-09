#pragma once
#include <cstdint>

struct alignas(64) RawOrder
{
    // Server-assigned sequence for strict dispatch ordering.
    uint64_t sequence;
    uint64_t order_id;
    uint32_t user_id;
    uint32_t symbol_id;

    int64_t price;
    uint32_t qty;

    uint8_t side;
    uint8_t type;

    uint64_t timestamp;
};
