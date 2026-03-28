#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

// Per-symbol open/close state.
class MarketState
{
private:
    std::vector<std::atomic<uint8_t>> open_;

public:
    inline explicit MarketState(size_t numSymbols, bool defaultOpen = true)
        : open_(numSymbols)
    {
        const uint8_t v = defaultOpen ? 1u : 0u;
        for (auto &a : open_)
        {
            a.store(v, std::memory_order_relaxed);
        }
    }

    inline bool isSymbolOpen(uint32_t symbolId) const noexcept
    {
        if (symbolId >= open_.size())
            return false;
        return open_[symbolId].load(std::memory_order_acquire) != 0;
    }

    inline void setSymbolOpen(uint32_t symbolId, bool isOpen) noexcept
    {
        if (symbolId >= open_.size())
            return;
        open_[symbolId].store(isOpen ? 1u : 0u, std::memory_order_release);
    }
};
