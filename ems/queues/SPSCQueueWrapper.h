#pragma once
#include <cstddef>
#include <rigtorp/SPSCQueue.h>
#include "../types/RawOrder.h"

// Wrapper over rigtorp::SPSCQueue preserving EMS queue interface.
class SPSCQueueWrapper
{
private:
    std::unique_ptr<rigtorp::SPSCQueue<RawOrder>> queue_;
    RawOrder staged_{};

public:
    inline explicit SPSCQueueWrapper(size_t size)
        : queue_(std::make_unique<rigtorp::SPSCQueue<RawOrder>>(size))
    {
    }

    SPSCQueueWrapper(const SPSCQueueWrapper &) = delete;
    SPSCQueueWrapper &operator=(const SPSCQueueWrapper &) = delete;
    SPSCQueueWrapper(SPSCQueueWrapper &&) noexcept = default;
    SPSCQueueWrapper &operator=(SPSCQueueWrapper &&) noexcept = default;

    inline RawOrder *claimSlot() noexcept
    {
        // We stage into stack-owned memory and publish atomically via underlying queue.
        return &staged_;
    }

    inline void publish() noexcept
    {
        queue_->push(staged_);
    }

    inline bool pop(RawOrder *&out) noexcept
    {
        RawOrder *front = queue_->front();
        if (front == nullptr)
            return false;
        out = front;
        return true;
    }

    inline void releaseSlot() noexcept
    {
        queue_->pop();
    }
};
