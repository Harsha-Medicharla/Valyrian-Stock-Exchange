#pragma once
#include <memory>
#include <cstddef>
#include <rigtorp/SPSCQueue.h>

template <typename T>
class EventSPSC
{
private:
    std::unique_ptr<rigtorp::SPSCQueue<T>> queue_;

public:
    explicit EventSPSC(std::size_t capacity)
        : queue_(std::make_unique<rigtorp::SPSCQueue<T>>(capacity))
    {
    }

    EventSPSC(const EventSPSC &) = delete;
    EventSPSC &operator=(const EventSPSC &) = delete;
    EventSPSC(EventSPSC &&) noexcept = default;
    EventSPSC &operator=(EventSPSC &&) noexcept = default;

    [[nodiscard]] inline bool tryPush(const T &event) noexcept
    {
        return queue_->try_push(event);
    }

    [[nodiscard]] inline T *front() noexcept
    {
        return queue_->front();
    }

    inline void pop() noexcept
    {
        queue_->pop();
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return queue_->empty();
    }
};
