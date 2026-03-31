#include "ConfirmationGenerator.h"
#include "../common/Pool.h"
#include <cstring>

void ConfirmationGenerator::generateAndPush(const Trade* trade, int32_t buy_rem, int32_t sell_rem, Pool<Confirmation>* q5) {
    uint64_t trade_value = trade->exec_price * trade->exec_qty;

    // Buyer Confirmation (Conf_A)
    Confirmation* confA = q5->allocate();
    if (confA) {
        confA->trade_id = trade->trade_id;
        confA->user_id = trade->buy_user_id;
        confA->order_id = trade->buy_order_id;
        std::strncpy(confA->symbol, trade->symbol, 8);
        confA->exec_price = trade->exec_price;
        confA->exec_qty = trade->exec_qty;
        confA->remaining_qty = buy_rem;
        confA->fund_delta = -(trade_value); 
        confA->share_delta = trade->exec_qty;
        confA->side = SIDE_BUY; 
        confA->status = (buy_rem == 0) ? STATUS_FILLED : STATUS_PARTIAL;
        q5->push(confA);
    }

    // Seller Confirmation (Conf_B)
    Confirmation* confB = q5->allocate();
    if (confB) {
        confB->trade_id = trade->trade_id;
        confB->user_id = trade->sell_user_id;
        confB->order_id = trade->sell_order_id;
        std::strncpy(confB->symbol, trade->symbol, 8);
        confB->exec_price = trade->exec_price;
        confB->exec_qty = trade->exec_qty;
        confB->remaining_qty = sell_rem;
        confB->fund_delta = trade_value; 
        confB->share_delta = -(trade->exec_qty);
        confB->side = SIDE_SELL; 
        confB->status = (sell_rem == 0) ? STATUS_FILLED : STATUS_PARTIAL;
        q5->push(confB);
    }
}