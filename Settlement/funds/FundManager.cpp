#include "FundManager.h"

void FundManager::processBuyer(uint64_t user_id, uint64_t trade_value) {
    auto& user_fund = in_memory_funds[user_id];
    user_fund.blocked_cash -= trade_value;
}

void FundManager::processSeller(uint64_t user_id, uint64_t trade_value) {
    auto& user_fund = in_memory_funds[user_id];
    user_fund.available_cash += trade_value;
}