#pragma once
#include "../entities/Trade.h"
#include <unordered_map>
#include <string>

struct Holding {
    int64_t qty = 0;
    int64_t blocked = 0;
};

class ShareManager {
public:
    void unblockSeller(Trade* t);
    void transferShares(Trade* t);
    bool reserveShares(uint64_t user_id, uint64_t symbol, int64_t qty);
    void releaseShares(uint64_t user_id, uint64_t symbol, int64_t qty);

private:
    std::unordered_map<uint64_t, std::unordered_map<std::string, Holding>> holdings;
};