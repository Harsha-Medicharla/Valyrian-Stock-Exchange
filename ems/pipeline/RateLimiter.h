#pragma once
#include <cstdint>

// Per-worker rate limiter.
// Hot path is lock-free and allocation-free; only a simple counter is updated.
class RateLimiter
{
private:
    uint64_t maxInWindow_;
    uint64_t decayFactor_;
    uint64_t counter_;

public:
    inline explicit RateLimiter(uint64_t maxInWindow = 100000, uint64_t decayFactor = 2) noexcept
        : maxInWindow_(maxInWindow),
          decayFactor_(decayFactor),
          counter_(0)
    {
    }

    // Returns whether the event is allowed.
    // `userId` is currently not used for indexing; the limiter is per-worker.
    inline bool checkAndIncrement(uint32_t /*userId*/) noexcept
    {
        if (counter_ >= maxInWindow_)
            return false;
        ++counter_;
        return true;
    }

    // Decays the counter (called periodically by the worker).
    inline void decay() noexcept
    {
        counter_ /= decayFactor_;
    }
};
