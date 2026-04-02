#pragma once
#include "../entities/Trade.h"
#include "../entities/Confirmation.h"
#include "../core/PartialFillHandler.h"

#include <utility>

class ConfirmationGenerator {
public:
    std::pair<Confirmation*, Confirmation*> generate(Trade* t, FillResult r);

private:
    Confirmation* createBuy(Trade* t, FillResult r);
    Confirmation* createSell(Trade* t, FillResult r);
};