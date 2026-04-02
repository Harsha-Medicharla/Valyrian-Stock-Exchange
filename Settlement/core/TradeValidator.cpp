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

    if (seen_ids.count(t->trade_id))
        return false;

    seen_ids.insert(t->trade_id);
    return true;
}