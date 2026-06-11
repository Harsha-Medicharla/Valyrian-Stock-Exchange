#pragma once
#include <cstddef>
#include <rigtorp/SPSCQueue.h>
#include "../types/RawOrder.h"

class SPSCQueueWrapper
{
private:
    std::unique_ptr<rigtorp::SPSCQueue<RawOrder>> queue_;

public:
    inline explicit SPSCQueueWrapper(size_t size)
        : queue_(std::make_unique<rigtorp::SPSCQueue<RawOrder>>(size))
    {
    }

    SPSCQueueWrapper(const SPSCQueueWrapper &) = delete;
    SPSCQueueWrapper &operator=(const SPSCQueueWrapper &) = delete;
    SPSCQueueWrapper(SPSCQueueWrapper &&) noexcept = default;
    SPSCQueueWrapper &operator=(SPSCQueueWrapper &&) noexcept = default;

    inline bool enqueue(const RawOrder &order) noexcept
    {
        return queue_->try_push(order);
    }

    [[nodiscard]] inline bool pop(RawOrder *&out) noexcept
    {
        out = queue_->front();
        return out != nullptr;
    }

    inline void releaseSlot() noexcept
    {
        queue_->pop();
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return queue_->empty();
    }
};
