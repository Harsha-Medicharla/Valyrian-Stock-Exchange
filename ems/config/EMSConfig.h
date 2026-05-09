#pragma once
#include <cstddef>
#include <cstdint>

// Single compile-time configuration for the EMS hot path and sizing.
namespace EMSConfig
{
    // Fixed capacity for lock-free per-user counters in RateLimiter.
    inline constexpr std::size_t MAX_USERS = 1u << 20;

    // Notional (price × quantity) must not exceed this limit (fat-finger protection).
    inline constexpr int64_t FAT_FINGER_LIMIT = 1'000'000'000'000LL;

    // Approximate per-user cap used by RateLimiter (decayed periodically by workers).
    inline constexpr uint64_t MAX_ORDERS_PER_SEC = 100'000;

    inline constexpr int64_t TICK_SIZE = 1;
    inline constexpr uint32_t LOT_SIZE = 1;

    inline constexpr std::size_t SPSC_BUFFER_SIZE = 1u << 12;
    inline constexpr std::size_t MPSC_BUFFER_SIZE = 1u << 12;
    inline constexpr std::size_t REJECTION_WORDS = MPSC_BUFFER_SIZE / 64;

    inline constexpr int DISPATCHER_BASE_CORE = 2;

    inline constexpr int64_t MIN_PRICE = 1;
    inline constexpr int64_t MAX_PRICE = 1'000'000'000;
    inline constexpr uint32_t MIN_QTY = 1;
    inline constexpr uint32_t MAX_QTY = 1'000'000'000;

}
