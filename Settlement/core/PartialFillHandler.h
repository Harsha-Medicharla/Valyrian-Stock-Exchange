// core/PartialFillHandler.h
#pragma once
#include "../entities/Trade.h"
#include <cstdint>

class PartialFillHandler {
public:
    void process(const Trade* trade, int32_t buy_remaining, int32_t sell_remaining);
};