// funds/FundManager.h
#pragma once
#include <cstdint>
#include <unordered_map>

struct UserFund {
    int64_t available_cash = 0;
    int64_t blocked_cash = 0;
};

class FundManager {
public:
    void processBuyer(uint64_t user_id, uint64_t trade_value);
    void processSeller(uint64_t user_id, uint64_t trade_value);

private:
    std::unordered_map<uint64_t, UserFund> in_memory_funds;
};