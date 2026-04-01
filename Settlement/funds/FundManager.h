#pragma once
#include <cstdint>
#include <unordered_map>
#include <mutex> 
#include <shared_mutex>
#include <vector>

namespace SettlementCore { // Renamed from Settlement

using Money = int64_t; 

struct UserBalance {
    Money available = 0;
    Money reserved = 0;
};

class FundManager {
private:
    std::unordered_map<uint64_t, UserBalance> balances;
    mutable std::shared_mutex rw_mutex;

public:
    FundManager() = default;

    // These must be declared here so the .cpp can implement them
    void processBuyer(uint64_t user_id, Money trade_value);
    void processSeller(uint64_t user_id, Money trade_value);

    bool reserveFunds(uint64_t user_id, Money amount) {
        std::unique_lock lock(rw_mutex);
        auto& bal = balances[user_id];
        if (bal.available < amount) return false;
        bal.available -= amount;
        bal.reserved += amount;
        return true;
    }

    void releaseFunds(uint64_t user_id, Money amount) {
        std::unique_lock lock(rw_mutex);
        auto& bal = balances[user_id];
        bal.reserved -= amount;
        bal.available += amount;
    }

    void transferFunds(uint64_t buyer_id, uint64_t seller_id, Money amount) {
        std::unique_lock lock(rw_mutex);
        balances[buyer_id].reserved -= amount;
        balances[seller_id].available += amount;
    }

    void deposit(uint64_t user_id, Money amount) {
        std::unique_lock lock(rw_mutex);
        balances[user_id].available += amount;
    }

    Money getAvailable(uint64_t user_id) const {
        std::shared_lock lock(rw_mutex);
        auto it = balances.find(user_id);
        return (it != balances.end()) ? it->second.available : 0;
    }
};

} // namespace SettlementCore