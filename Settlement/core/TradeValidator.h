#pragma once
#include "../entities/Trade.h"
#include <unordered_set>

class TradeValidator {
public:
    bool validate(Trade* t);

private:
    std::unordered_set<uint64_t> seen_ids;

    uint32_t computeChecksum(Trade* t);
};