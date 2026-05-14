#pragma once
#include <memory>
#include <cstddef>
#include <rigtorp/SPSCQueue.h>

// Lock-free single-producer single-consumer queue for outbound engine events.
// Template parameter T must be trivially copyable and alignas(64).
//
// Writer (MatchingEngine): calls tryPush(). Non-blocking — drops if full.
//   The engine MUST NEVER block on output. A lagging consumer loses events;
//   the WAL is the recovery source of truth.
//
// Reader (consumer thread): calls front() / pop() in a drain loop.
//
// This is the same rigtorp::SPSCQueue used by SPSCQueueWrapper. The interface
// is intentionally identical so all queue types look the same to their users.

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

    // Called by the engine (writer). Returns false and drops the event if full.
    [[nodiscard]] inline bool tryPush(const T &event) noexcept
    {
        return queue_->try_push(event);
    }

    // Called by the consumer (reader). Returns nullptr if empty.
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
