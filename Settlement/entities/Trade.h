#ifndef TRADE_H
#define TRADE_H

#include <cstdint>

struct Trade {
    uint64_t  trade_id;
    uint64_t  buy_order_id;
    uint64_t  sell_order_id;
    uint64_t  buy_user_id;
    uint64_t  sell_user_id;
    char      symbol[8];
    int64_t   exec_price;      // in paise, not float
    int32_t   exec_qty;
    int32_t   buy_total_qty;
    int32_t   sell_total_qty;
    uint64_t  timestamp;
    uint32_t  checksum;
};

#endif