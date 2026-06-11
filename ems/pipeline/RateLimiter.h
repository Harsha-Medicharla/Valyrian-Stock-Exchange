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

class RateLimiter
{
private:
    struct alignas(64) UserGroup
    {
        struct Slot
        {
            std::atomic<uint32_t> count{0};
            std::atomic<uint32_t> epoch{0};
        } slots[4];
    };

    static constexpr std::size_t kGroupSize = 4;
    static constexpr std::size_t kNumGroups = EMSConfig::MAX_USERS / kGroupSize;

    uint32_t maxInWindow_;
    std::unique_ptr<UserGroup[]> groups_;

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
        if ((globalOps_.fetch_add(1, std::memory_order_relaxed) & 1023) != 0)
            return;
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

    inline UserGroup::Slot &getSlot(uint32_t userId) noexcept
    {
        const std::size_t idx = static_cast<std::size_t>(userId) % EMSConfig::MAX_USERS;
        const std::size_t group = idx / kGroupSize;
        const std::size_t slot = idx % kGroupSize;
        return groups_[group].slots[slot];
    }

public:
    inline explicit RateLimiter(
        uint32_t maxInWindow = static_cast<uint32_t>(EMSConfig::MAX_ORDERS_PER_SEC)) noexcept
        : maxInWindow_(maxInWindow),
          groups_(std::make_unique<UserGroup[]>(kNumGroups))
    {
        epochStartMs_.store(coarseTimeMs(), std::memory_order_relaxed);
    }

    inline bool checkAndIncrement(uint32_t userId) noexcept
    {
        maybeAdvanceEpoch();

        const uint64_t epoch = currentEpoch_.load(std::memory_order_relaxed);
        UserGroup::Slot &entry = getSlot(userId);

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
