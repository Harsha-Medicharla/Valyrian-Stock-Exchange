#include "PartialFillHandler.h"
#include <iostream>

void PartialFillHandler::process(const Trade* trade, int32_t buy_remaining, int32_t sell_remaining) {
    // If we needed Immediate-Or-Cancel expiration, we'd release funds here.
    // Otherwise, standard limit orders handle partials inside the ME.
    if (buy_remaining > 0 || sell_remaining > 0) {
        // Just leaving this here if you want to track partials later
    }
}