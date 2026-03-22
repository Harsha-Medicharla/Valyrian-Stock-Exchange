#pragma once
#include <cstdint>
#include <unordered_set>
#include <mutex>

namespace EMS {

class MarketState {
public:
    // Definition is here (Inline)
    bool isSymbolOpen(uint64_t symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return open_symbols_.find(symbol) != open_symbols_.end();
    }

    void openSymbol(uint64_t symbol) {
        std::lock_guard<std::mutex> lock(mutex_);
        open_symbols_.insert(symbol);
    }

private:
    std::unordered_set<uint64_t> open_symbols_;
    mutable std::mutex mutex_;
};

}