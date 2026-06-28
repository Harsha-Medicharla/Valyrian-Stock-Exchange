#pragma once
#include <atomic>
#include <cstdint>

class IdGenerator
{
private:
    std::atomic<uint64_t> next_;

public:
    inline explicit IdGenerator(uint64_t start = 1) noexcept : next_(start) {}

    inline uint64_t nextId() noexcept
    {
        return next_.fetch_add(1, std::memory_order_relaxed);
    }
};
