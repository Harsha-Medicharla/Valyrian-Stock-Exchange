// entities/Confirmation.h
#pragma once
#include <cstdint>

struct Confirmation {
    uint64_t  trade_id;
    uint64_t  user_id;
    uint64_t  order_id;
    char      symbol[8];
    int64_t   exec_price;
    int32_t   exec_qty;
    int32_t   remaining_qty;
    int64_t   fund_delta;
    int32_t   share_delta;
    uint8_t   side;            // 0 for BUY, 1 for SELL
    uint8_t   status;          // FILLED / PARTIAL / REJECTED
};