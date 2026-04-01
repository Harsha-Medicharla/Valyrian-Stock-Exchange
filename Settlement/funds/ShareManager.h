#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <shared_mutex> // Added for thread safety

namespace SettlementCore {

struct UserHolding {
    int32_t available_qty = 0;
    int32_t blocked_qty = 0;
};

class ShareManager {
public:
    // 1. FOR TESTS: Give a user initial shares to sell
    void deposit(uint64_t user_id, uint64_t symbol, int32_t qty) {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        in_memory_holdings[user_id][symbol].available_qty += qty;
    }

    // 2. PRE-TRADE: Lock shares when a SELL order is placed
    bool reserve(uint64_t user_id, uint64_t symbol, int32_t qty) {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        auto& holding = in_memory_holdings[user_id][symbol];
        
        if (holding.available_qty >= qty) {
            holding.available_qty -= qty;
            holding.blocked_qty += qty;
            return true;
        }
        return false; // User doesn't have enough shares to sell!
    }

    // 3. PRE-TRADE: Unlock shares if a SELL order is canceled
    void release(uint64_t user_id, uint64_t symbol, int32_t qty) {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        auto& holding = in_memory_holdings[user_id][symbol];
        
        holding.blocked_qty -= qty;
        holding.available_qty += qty;
    }

    // 4. POST-TRADE: Your original transfer method, updated for thread-safety and uint64_t
    void transferShares(uint64_t seller_id, uint64_t buyer_id, uint64_t symbol, int32_t qty) {
        std::unique_lock<std::shared_mutex> lock(rw_lock);
        
        // Take from seller's blocked inventory
        in_memory_holdings[seller_id][symbol].blocked_qty -= qty;
        // Give to buyer's available inventory
        in_memory_holdings[buyer_id][symbol].available_qty += qty;
    }

private:
    // Swapped std::string to uint64_t to match your engine's symbol router
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, UserHolding>> in_memory_holdings;
    std::shared_mutex rw_lock;
};

} // namespace SettlementCore