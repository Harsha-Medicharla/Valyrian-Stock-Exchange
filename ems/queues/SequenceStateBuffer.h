#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "../config/EMSConfig.h"
#include "../types/OrderSlot.h"
#include "../utils/SpinWait.h"

// Fixed-size lock-free helper for server-sequence decisions (rejected/skip).
// Workers publish rejected sequences; dispatcher consumes by expected server sequence.
class SequenceStateBuffer
{
public:
    SequenceStateBuffer() noexcept
        : entries_(std::make_unique<Entry[]>(EMSConfig::MPSC_BUFFER_SIZE))
    {
        for (std::size_t i = 0; i < EMSConfig::MPSC_BUFFER_SIZE; ++i)
        {
            entries_[i].seq.store(0, std::memory_order_relaxed);
            entries_[i].state.store(static_cast<uint8_t>(OrderSlotState::Pending), std::memory_order_relaxed);
        }
    }

    SequenceStateBuffer(const SequenceStateBuffer &) = delete;
    SequenceStateBuffer &operator=(const SequenceStateBuffer &) = delete;
    SequenceStateBuffer(SequenceStateBuffer &&) noexcept = default;
    SequenceStateBuffer &operator=(SequenceStateBuffer &&) noexcept = default;

    inline void markRejected(uint64_t serverSeq) noexcept
    {
        if (serverSeq == 0)
            return;

        Entry &entry = entries_[indexOf(serverSeq)];
        while (entry.seq.load(std::memory_order_acquire) != 0)
        {
            ems::pause();
        }

        entry.state.store(static_cast<uint8_t>(OrderSlotState::Rejected), std::memory_order_relaxed);
        entry.seq.store(serverSeq, std::memory_order_release);
    }

    [[nodiscard]] inline bool tryConsumeRejected(uint64_t expectedSeq) noexcept
    {
        if (expectedSeq == 0)
            return false;

        Entry &entry = entries_[indexOf(expectedSeq)];
        if (entry.seq.load(std::memory_order_acquire) != expectedSeq)
            return false;

        if (static_cast<OrderSlotState>(entry.state.load(std::memory_order_relaxed)) != OrderSlotState::Rejected)
            return false;

        entry.state.store(static_cast<uint8_t>(OrderSlotState::Pending), std::memory_order_relaxed);
        entry.seq.store(0, std::memory_order_release);
        return true;
    }

private:
    struct alignas(64) Entry
    {
        std::atomic<uint64_t> seq{0};
        std::atomic<uint8_t> state{static_cast<uint8_t>(OrderSlotState::Pending)};
    };

    static constexpr std::size_t kMask = EMSConfig::MPSC_BUFFER_SIZE - 1;

    static inline constexpr std::size_t indexOf(uint64_t seq) noexcept
    {
        return static_cast<std::size_t>(seq) & kMask;
    }

    std::unique_ptr<Entry[]> entries_;
};
