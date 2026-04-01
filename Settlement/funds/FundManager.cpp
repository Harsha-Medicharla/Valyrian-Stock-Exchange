#include "Settlement/funds/FundManager.h"
#include <mutex>

namespace SettlementCore {

void FundManager::processBuyer(uint64_t user_id, Money trade_value) {
    std::unique_lock lock(rw_mutex);
    balances[user_id].reserved -= trade_value;
}

void FundManager::processSeller(uint64_t user_id, Money trade_value) {
    std::unique_lock lock(rw_mutex);
    balances[user_id].available += trade_value;
}

} // namespace SettlementCore