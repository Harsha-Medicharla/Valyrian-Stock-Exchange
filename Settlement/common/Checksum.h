// common/Checksum.h
#pragma once
#include "../entities/Trade.h"
#include <cstdint>

class Checksum {
public:
    static uint32_t calculate(const Trade* trade) {
        // Simple fast hash (e.g., FNV-1a or similar) over critical fields
        uint32_t hash = 2166136261u;
        auto hash_combine = [&hash](uint64_t val) {
            hash ^= val;
            hash *= 16777619;
        };
        hash_combine(trade->trade_id);
        hash_combine(trade->buy_user_id);
        hash_combine(trade->sell_user_id);
        hash_combine(trade->exec_price);
        hash_combine(trade->exec_qty);
        return hash;
    }
};