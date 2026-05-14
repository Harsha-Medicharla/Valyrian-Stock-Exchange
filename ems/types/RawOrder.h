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

    uint8_t cancel_flag;   // 1 = this is a cancel request; order_id identifies the target
    uint8_t modify_flag;   // 1 = this is a modify request; price and qty are new values

    uint64_t timestamp;
};

static_assert(sizeof(RawOrder) == 64, "RawOrder must remain 64 bytes");
