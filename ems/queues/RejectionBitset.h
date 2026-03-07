#pragma once
#include <atomic>
#include <cstdint>
#include "../config/EMSConfig.h"

// Lock-free rejection signal channel — replaces SequenceStateBuffer.
// Uses a 256-bit sliding window of atomic bits, one bit per server sequence.
// markRejected()     — single fetch_or, no spin, wait-free on the hot path.
// tryConsumeRejected — single load + fetch_and, called only by the Dispatcher.
//
// Safety invariant: the Dispatcher must consume sequences faster than producers
// can lap the 256-sequence window. This holds because the Dispatcher is a pinned
// spin-loop and 256 > any realistic burst of unprocessed rejections.
struct alignas(64) RejectionBitset
{
    static constexpr std::size_t kWords = EMSConfig::REJECTION_WORDS;

    std::atomic<uint64_t> words_[kWords]{};

    // Called by IngressWorker on the hot path. Wait-free — single atomic OR.
    inline void markRejected(uint64_t serverSeq) noexcept
    {
        const uint64_t zero_based = serverSeq - 1;
        const std::size_t wordIdx = (zero_based / 64) % kWords;
        const uint64_t bitMask = 1ULL << (zero_based % 64);
        words_[wordIdx].fetch_or(bitMask, std::memory_order_release);
    }

    // Called by Dispatcher. Returns true and clears the bit if this sequence
    // was marked rejected; returns false if it was not.
    [[nodiscard]] inline bool tryConsumeRejected(uint64_t expectedSeq) noexcept
    {
        const uint64_t zero_based = expectedSeq - 1;
        const std::size_t wordIdx = (zero_based / 64) % kWords;
        const uint64_t bitMask = 1ULL << (zero_based % 64);

        if (!(words_[wordIdx].load(std::memory_order_acquire) & bitMask))
            return false;

        words_[wordIdx].fetch_and(~bitMask, std::memory_order_release);
        return true;
    }
};
