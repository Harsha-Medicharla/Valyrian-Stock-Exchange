#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "../config/EMSConfig.h"

// Shared per-user rate limiter using fixed-size lock-free atomic counters.
class RateLimiter
{
private:
    uint64_t maxInWindow_;
    uint64_t decayFactor_;
    std::array<std::atomic<uint32_t>, EMSConfig::MAX_USERS> counters_;

public:
    inline explicit RateLimiter(
        uint64_t maxInWindow = EMSConfig::MAX_ORDERS_PER_SEC,
        uint64_t decayFactor = EMSConfig::RATE_LIMITER_DECAY_FACTOR) noexcept
        : maxInWindow_(maxInWindow),
          decayFactor_(decayFactor),
          counters_()
    {
        for (auto &c : counters_)
        {
            c.store(0, std::memory_order_relaxed);
        }
    }

    inline bool checkAndIncrement(uint32_t userId) noexcept
    {
        const std::size_t idx = static_cast<std::size_t>(userId) % EMSConfig::MAX_USERS;
        std::atomic<uint32_t> &counter = counters_[idx];

        uint32_t current = counter.load(std::memory_order_relaxed);
        while (true)
        {
            if (current >= maxInWindow_)
                return false;

            if (counter.compare_exchange_weak(
                    current,
                    current + 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed))
            {
                return true;
            }
        }

        return false;
    }

    inline void decay() noexcept
    {
        if (decayFactor_ == 0)
            return;

        const uint32_t factor = static_cast<uint32_t>(decayFactor_);
        for (auto &counter : counters_)
        {
            uint32_t current = counter.load(std::memory_order_relaxed);
            while (true)
            {
                const uint32_t decayed = current / factor;
                if (counter.compare_exchange_weak(
                        current,
                        decayed,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed))
                {
                    break;
                }
            }
        }
    }
};
