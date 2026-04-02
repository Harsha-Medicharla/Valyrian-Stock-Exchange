#include "TradeValidator.h"

uint32_t TradeValidator::computeChecksum(Trade* t) {
    return (uint32_t)(
        t->trade_id ^
        t->buyer_id ^
        t->seller_id ^
        t->symbol ^
        t->price ^
        t->qty
    );
}

bool TradeValidator::validate(Trade* t) {
    if (computeChecksum(t) != t->checksum)
        return false;

    return true;
}