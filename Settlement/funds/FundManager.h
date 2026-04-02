#pragma once
#include "../entities/Trade.h"
#include <unordered_map>

struct FundState {
    int64_t balance = 0;
    int64_t blocked = 0;
};

class FundManager {
public:
    void unblockBuyer(Trade* t);
    void debitBuyer(Trade* t);
    void creditSeller(Trade* t);
    bool reserveFunds(uint64_t user_id, int64_t amount);
    void releaseFunds(uint64_t user_id, int64_t amount);

private:
    std::unordered_map<uint64_t, FundState> funds;
};