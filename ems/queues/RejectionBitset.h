#pragma once
#include <atomic>
#include <cstdint>
#include "../config/EMSConfig.h"

struct alignas(64) RejectionBitset
{
    static constexpr std::size_t kWords = EMSConfig::REJECTION_WORDS;

    std::atomic<uint64_t> words_[kWords]{};

    inline void markRejected(uint64_t serverSeq) noexcept
    {
        const uint64_t zero_based = serverSeq - 1;
        const std::size_t wordIdx = (zero_based / 64) % kWords;
        const uint64_t bitMask = 1ULL << (zero_based % 64);
        words_[wordIdx].fetch_or(bitMask, std::memory_order_release);
    }

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
