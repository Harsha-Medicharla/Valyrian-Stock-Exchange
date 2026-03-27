#pragma once
#include <cstddef>
#include <cstdint>

namespace EMSConfig
{
    inline constexpr std::size_t MAX_USERS = 1u << 17;

    inline constexpr int64_t FAT_FINGER_LIMIT = 1'000'000'000'000LL;

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
