#ifndef SETTLEMENT_H
#define SETTLEMENT_H

#include "Settlement/funds/FundManager.h" 
#include <cstdint>

namespace SettlementCore { // Renamed to match the module

class Settlement {
public:
    void processQueue();
    void settleTrade(uint64_t buyer_id, uint64_t seller_id, uint64_t symbol, int64_t price, int32_t qty);
    bool reserveMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty);
    void releaseMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty);
    void adminDeposit(uint64_t user_id, int64_t amount) {
        fund_manager.deposit(user_id, amount);
    }
private:
    FundManager fund_manager; 
    bool q4_trades = true; 
};

} // namespace SettlementCore

#endif