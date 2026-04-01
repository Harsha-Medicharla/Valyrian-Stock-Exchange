// core/TradeValidator.h
#pragma once
#include "../entities/Trade.h"
#include "../common/Checksum.h"
#include <unordered_set>

class TradeValidator {
public:
    bool isValid(const Trade* trade);
    Trade* recoverFromWAL(uint64_t trade_id);

private:
    std::unordered_set<uint64_t> processed_trades;
};