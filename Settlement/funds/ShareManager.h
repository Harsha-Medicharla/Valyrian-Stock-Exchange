// funds/ShareManager.h
#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>

struct UserHolding {
    int32_t available_qty = 0;
    int32_t blocked_qty = 0;
};

class ShareManager {
public:
    void transferShares(uint64_t seller_id, uint64_t buyer_id, const char* symbol, int32_t qty);

private:
    // map[user_id][symbol] -> UserHolding
    std::unordered_map<uint64_t, std::unordered_map<std::string, UserHolding>> in_memory_holdings;
};