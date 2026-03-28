#pragma once
#include <atomic>
#include <cstdint>
#include <disruptorplus/multi_threaded_claim_strategy.hpp>
#include <disruptorplus/ring_buffer.hpp>
#include <disruptorplus/sequence.hpp>
#include <disruptorplus/sequence_barrier.hpp>
#include <disruptorplus/spin_wait_strategy.hpp>
#include "../types/OrderSlot.h"

class MPSCRingBuffer
{
private:
    struct Impl
    {
        inline explicit Impl(size_t size)
            : buffer(normalizeSize(size)),
              waitStrategy(),
              claimStrategy(buffer.size(), waitStrategy),
              consumedBarrier(waitStrategy)
        {
            claimStrategy.add_claim_barrier(consumedBarrier);
        }

        inline static size_t normalizeSize(size_t requested) noexcept
        {
            if (requested < 2)
                requested = 2;
            size_t s = 1;
            while (s < requested)
                s <<= 1;
            return s;
        }

        disruptorplus::ring_buffer<OrderSlot> buffer;
        disruptorplus::spin_wait_strategy waitStrategy;
        disruptorplus::multi_threaded_claim_strategy<disruptorplus::spin_wait_strategy> claimStrategy;
        disruptorplus::sequence_barrier<disruptorplus::spin_wait_strategy> consumedBarrier;
    };

    inline void updatePublishedCursor(uint64_t seq) noexcept
    {
        uint64_t observed = publishedCursor_.load(std::memory_order_relaxed);
        while (observed < seq &&
               !publishedCursor_.compare_exchange_weak(
                   observed,
                   seq,
                   std::memory_order_release,
                   std::memory_order_relaxed))
        {
        }
    }
    std::unique_ptr<Impl> impl_;
    alignas(64) std::atomic<uint64_t> publishedCursor_{0};

public:
    inline explicit MPSCRingBuffer(size_t size)
        : impl_(std::make_unique<Impl>(size))
    {
    }

    MPSCRingBuffer(const MPSCRingBuffer &) = delete;
    MPSCRingBuffer &operator=(const MPSCRingBuffer &) = delete;
    MPSCRingBuffer(MPSCRingBuffer &&other) noexcept
        : impl_(std::move(other.impl_)),
          publishedCursor_(other.publishedCursor_.load(std::memory_order_relaxed))
    {
    }
    MPSCRingBuffer &operator=(MPSCRingBuffer &&) noexcept = delete;

    inline OrderSlot *claimSlot(uint64_t &seq)
    {
        const disruptorplus::sequence_t internalSeq = impl_->claimStrategy.claim_one();
        seq = static_cast<uint64_t>(internalSeq) + 1; // external EMS sequence is 1-based
        return &impl_->buffer[internalSeq];
    }

    inline void publish(uint64_t seq)
    {
        const disruptorplus::sequence_t internalSeq =
            static_cast<disruptorplus::sequence_t>(seq - 1);
        impl_->claimStrategy.publish(internalSeq);
        updatePublishedCursor(seq);
    }

    inline bool isAvailable(uint64_t seq) const
    {
        if (seq == 0)
            return false;
        const auto internalSeq = static_cast<disruptorplus::sequence_t>(seq - 1);
        const auto lastKnown = (internalSeq == 0)
                                   ? static_cast<disruptorplus::sequence_t>(-1)
                                   : static_cast<disruptorplus::sequence_t>(internalSeq - 1);
        const auto highest = impl_->claimStrategy.last_published_after(lastKnown);
        return disruptorplus::difference(highest, internalSeq) >= 0;
    }

    inline uint64_t getHighestPublished(uint64_t next, uint64_t cursor) const
    {
        if (next > cursor)
            return next - 1;

        const auto internalNext = static_cast<disruptorplus::sequence_t>(next - 1);
        const auto lastKnown = (internalNext == 0)
                                   ? static_cast<disruptorplus::sequence_t>(-1)
                                   : static_cast<disruptorplus::sequence_t>(internalNext - 1);

        const auto internalHighest = impl_->claimStrategy.last_published_after(lastKnown);
        const uint64_t externalHighest = static_cast<uint64_t>(internalHighest) + 1;
        return (externalHighest < cursor) ? externalHighest : cursor;
    }

    inline void releaseSlot(uint64_t seq)
    {
        const disruptorplus::sequence_t internalSeq =
            static_cast<disruptorplus::sequence_t>(seq - 1);
        impl_->consumedBarrier.publish(internalSeq);
    }

    inline uint64_t cursor() const
    {
        return publishedCursor_.load(std::memory_order_acquire);
    }

    inline OrderSlot &get(uint64_t seq)
    {
        return impl_->buffer[static_cast<disruptorplus::sequence_t>(seq - 1)];
    }
};
