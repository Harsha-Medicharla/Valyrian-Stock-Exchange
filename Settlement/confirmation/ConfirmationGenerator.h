// confirmation/ConfirmationGenerator.h
#pragma once
#include "../entities/Trade.h"
#include "../entities/Confirmation.h"
#include "../common/SettlementConstants.h"
#include <cstring>

template <typename T> class Pool;

class ConfirmationGenerator {
public:
    void generateAndPush(const Trade* trade, int32_t buy_rem, int32_t sell_rem, Pool<Confirmation>* q5);
};