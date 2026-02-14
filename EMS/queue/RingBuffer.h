#pragma once

#include <atomic>
#include <cstddef>
#include <array>

template<typename T, size_t Size>
class RingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");

public:
    RingBuffer() : head_(0), tail_(0) {}

    // Multiple Producers
    bool push(const T& item) {
        size_t head = head_.load(std::memory_order_relaxed);

        while (true) {
            size_t tail = tail_.load(std::memory_order_acquire);

            if (head - tail >= Size) {
                return false; // buffer full
            }

            if (head_.compare_exchange_weak(
                    head,
                    head + 1,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                break;
            }
        }

        buffer_[head & mask_] = item;
        return true;
    }

    // Single Consumer
    bool pop(T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);

        if (tail == head) {
            return false; // empty
        }

        item = buffer_[tail & mask_];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

private:
    static constexpr size_t mask_ = Size - 1;

    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
    alignas(64) std::array<T, Size> buffer_;
};
