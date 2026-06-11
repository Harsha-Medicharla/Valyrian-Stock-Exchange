#pragma once
#include <atomic>
#include <cstdint>
#include "shared/queues/EventSPSC.h"
#include "shared/types/Events.h"
#include "../types/Common.h"

struct RawOrder;

class RejectHandler
{
public:
    using RejectCallback = void (*)(const RawOrder *order, RejectReason reason) noexcept;

    static void registerCallback(RejectCallback cb) noexcept
    {
        callback_.store(cb, std::memory_order_release);
    }

    static void invoke(const RawOrder *order, RejectReason reason) noexcept
    {
        RejectCallback cb = callback_.load(std::memory_order_acquire);
        if (cb)
            cb(order, reason);
    }

    static void setRejectQueue(EventSPSC<OrderEvent> *queue) noexcept
    {
        currentRejectQueue() = queue;
    }

    [[nodiscard]] static EventSPSC<OrderEvent> *rejectQueue() noexcept
    {
        return currentRejectQueue();
    }

private:
    static EventSPSC<OrderEvent> *&currentRejectQueue() noexcept
    {
        static thread_local EventSPSC<OrderEvent> *queue = nullptr;
        return queue;
    }

    inline static std::atomic<RejectCallback> callback_{nullptr};
};
