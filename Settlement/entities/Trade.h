#ifndef TRADE_H
#define TRADE_H

#include <cstdint>

struct Trade {
    uint64_t trade_id;
    uint64_t buyer_id;
    uint64_t seller_id;
    uint64_t symbol;
    int64_t price;
    int32_t qty;
    uint64_t checksum; 
};

#endif