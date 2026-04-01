#include "Settlement/core/Settlement.h"
#include "Settlement/entities/Trade.h"
#include <iostream>

namespace SettlementCore { // <-- Added this to match the header

void Settlement::processQueue() {
    if (!q4_trades) return;
    
    // Future Part 2 implementation logic will go here
}

void Settlement::settleTrade(uint64_t buyer_id, uint64_t seller_id, uint64_t symbol, int64_t price, int32_t qty) {
    int64_t total_value = price * qty;
    
    fund_manager.transferFunds(buyer_id, seller_id, total_value);
    
    std::cout << "[SETTLEMENT] Transferred " << total_value 
              << " from Buyer " << buyer_id << " to Seller " << seller_id << "\n";
}

bool Settlement::reserveMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
    if (side == 0) { // BUY side requires cash
        int64_t total_cost = price * qty;
        return fund_manager.reserveFunds(user_id, total_cost);
    }
    return true; // SELL side logic pending ShareManager
}

void Settlement::releaseMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
    if (side == 0) {
        int64_t amount = price * qty;
        fund_manager.releaseFunds(user_id, amount);
    }
}

} // namespace SettlementCore // <-- Closed it here