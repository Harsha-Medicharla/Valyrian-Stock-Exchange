#pragma once
#include <atomic>
#include <cstdint>
#include "../types/Common.h"

class RejectHandler
{
public:
    using RejectCallback = void (*)(uint32_t userId, RejectReason reason) noexcept;

    // Register a global reject callback.
    // Default is nullptr (no-op).
    static void registerCallback(RejectCallback cb) noexcept
    {
        callback_.store(cb, std::memory_order_release);
    }

    // Direct invocation (no queues, no allocations).
    static void invoke(uint32_t userId, RejectReason reason) noexcept
    {
        RejectCallback cb = callback_.load(std::memory_order_acquire);
        if (cb)
            cb(userId, reason);
    }

private:
    inline static std::atomic<RejectCallback> callback_{nullptr};
};
