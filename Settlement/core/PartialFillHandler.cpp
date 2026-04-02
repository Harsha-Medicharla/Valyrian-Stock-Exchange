#include "PartialFillHandler.h"

#define FILLED 1

FillResult PartialFillHandler::handle(Trade* t) {
    FillResult res;

    res.buy_remaining = 0;
    res.sell_remaining = 0;

    res.buy_status = FILLED;
    res.sell_status = FILLED;

    return res;
}