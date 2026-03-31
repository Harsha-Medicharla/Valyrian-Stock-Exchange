#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <time.h>
#include "../config/EMSConfig.h"

#ifndef CLOCK_MONOTONIC_COARSE
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
#endif

// Per-user rate limiter with epoch-based lazy decay.
//
// Design:
//   - Each user has a cache-line-sized entry holding a count and an epoch.
//     alignas(64) eliminates false sharing between workers on different users.
//   - A shared currentEpoch_ counter is advanced at most once per second.
//     Any worker can advance it via a single CAS; all others see the new value
//     immediately and reset only their current user's counter — O(1) per order.
//   - decay() and the opsSinceDecay_ machinery in IngressWorker are removed.
//     There is no periodic stall anywhere.
//
// Determinism note: epoch advancement is wall-clock driven, so accept/reject
// decisions for borderline users are not reproducible under WAL replay at a
// different speed. This is acceptable: the rate limiter is explicitly approximate
// and the matching engine's order sequence remains fully deterministic.
class RateLimiter
{
private:
    struct alignas(64) UserEntry
    {
        std::atomic<uint32_t> count{0};
        std::atomic<uint32_t> epoch{0};
    };

    uint32_t maxInWindow_;
    std::unique_ptr<UserEntry[]> entries_;

    alignas(64) std::atomic<uint64_t> currentEpoch_{0};
    alignas(64) std::atomic<uint64_t> epochStartMs_{0};
    alignas(64) std::atomic<uint64_t> globalOps_{0};

    static inline uint64_t coarseTimeMs() noexcept
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * 1000ULL + static_cast<uint64_t>(ts.tv_nsec) / 1'000'000ULL;
    }

    inline void maybeAdvanceEpoch() noexcept
    {
        // Only touch the system clock every 1024 calls
        // This reduces vDSO/syscall overhead by 99.9%
        if ((globalOps_.fetch_add(1, std::memory_order_relaxed) & 1023) != 0)
        {
            return;
        }
        const uint64_t now = coarseTimeMs();
        const uint64_t epoch = currentEpoch_.load(std::memory_order_relaxed);
        if (now - epochStartMs_.load(std::memory_order_relaxed) < 1000ULL)
            return;
        uint64_t expected = epoch;
        if (currentEpoch_.compare_exchange_strong(
                expected, epoch + 1,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
        {
            epochStartMs_.store(now, std::memory_order_relaxed);
        }
    }

public:
    inline explicit RateLimiter(
        uint32_t maxInWindow = static_cast<uint32_t>(EMSConfig::MAX_ORDERS_PER_SEC)) noexcept
        : maxInWindow_(maxInWindow),
          entries_(std::make_unique<UserEntry[]>(EMSConfig::MAX_USERS))
    {
        epochStartMs_.store(coarseTimeMs(), std::memory_order_relaxed);
    }

    // Returns true and increments the counter if this user is under the limit.
    // Returns false and leaves the counter unchanged if the limit is reached.
    inline bool checkAndIncrement(uint32_t userId) noexcept
    {
        maybeAdvanceEpoch();

        const uint64_t epoch = currentEpoch_.load(std::memory_order_relaxed);
        UserEntry &entry = entries_[static_cast<std::size_t>(userId) % EMSConfig::MAX_USERS];

        // Lazily reset this user's counter if their epoch is stale.
        if (entry.epoch.load(std::memory_order_relaxed) != static_cast<uint32_t>(epoch))
        {
            entry.count.store(0, std::memory_order_relaxed);
            entry.epoch.store(static_cast<uint32_t>(epoch), std::memory_order_relaxed);
        }

        uint32_t current = entry.count.load(std::memory_order_relaxed);
        while (true)
        {
            if (current >= maxInWindow_)
                return false;
            if (entry.count.compare_exchange_weak(
                    current, current + 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed))
                return true;
        }
    }
};
