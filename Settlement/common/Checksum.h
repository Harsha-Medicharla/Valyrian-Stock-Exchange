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
        hash_combine(trade->buyer_id);
        hash_combine(trade->seller_id);
        hash_combine(trade->price);
        hash_combine(trade->qty);
        return hash;
    }
};