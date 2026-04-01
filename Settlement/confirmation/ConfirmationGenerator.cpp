#include "ConfirmationGenerator.h"
#include "../common/Pool.h"
#include <cstring>
#include <string>

void ConfirmationGenerator::generateAndPush(const Trade* trade, int32_t buy_rem, int32_t sell_rem, Pool<Confirmation>* q5) {
    // 1. Calculate total trade value using correct struct members
    int64_t trade_value = trade->price * trade->qty;

    // --- Buyer Confirmation (Conf_A) ---
    Confirmation* confA = q5->allocate();
    if (confA) {
        confA->trade_id = trade->trade_id;
        confA->user_id = trade->buyer_id;
        confA->order_id = trade->trade_id; // Using trade_id as placeholder for buy_order_id
        
        // Handle Symbol (Assuming Confirmation struct uses char[8])
        std::string sym_str = std::to_string(trade->symbol);
        std::strncpy(confA->symbol, sym_str.c_str(), 7);
        confA->symbol[7] = '\0'; // Ensure null termination

        confA->exec_price = trade->price;
        confA->exec_qty = trade->qty;
        confA->remaining_qty = buy_rem;
        confA->fund_delta = -(trade_value); 
        confA->share_delta = trade->qty;
        confA->side = SIDE_BUY; 
        confA->status = (buy_rem == 0) ? STATUS_FILLED : STATUS_PARTIAL;
        
        q5->push(confA);
    }

    // --- Seller Confirmation (Conf_B) ---
    Confirmation* confB = q5->allocate();
    if (confB) {
        confB->trade_id = trade->trade_id;
        confB->user_id = trade->seller_id;
        confB->order_id = trade->trade_id; // Using trade_id as placeholder for sell_order_id
        
        // Handle Symbol
        std::string sym_str = std::to_string(trade->symbol);
        std::strncpy(confB->symbol, sym_str.c_str(), 7);
        confB->symbol[7] = '\0';

        confB->exec_price = trade->price;
        confB->exec_qty = trade->qty;
        confB->remaining_qty = sell_rem;
        confB->fund_delta = trade_value; 
        confB->share_delta = -(trade->qty);
        confB->side = SIDE_SELL; 
        confB->status = (sell_rem == 0) ? STATUS_FILLED : STATUS_PARTIAL;
        
        q5->push(confB);
    }
}