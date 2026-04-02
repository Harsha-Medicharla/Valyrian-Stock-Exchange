#pragma once
#include "../entities/Trade.h"

struct FillResult {
    int32_t buy_remaining;
    int32_t sell_remaining;
    uint8_t buy_status;
    uint8_t sell_status;
};

class PartialFillHandler {
public:
    FillResult handle(Trade* t);
};