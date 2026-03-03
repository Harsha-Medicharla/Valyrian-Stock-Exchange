#pragma once
#include <cstdint>

enum class Side : uint8_t
{
    BUY,
    SELL
};

enum class OrderType : uint8_t
{
    LIMIT,
    MARKET
};

enum class Decision : uint8_t
{
    ACCEPT,
    REJECT
};

enum class RejectReason : uint8_t
{
    INVALID_RANGE,
    FAT_FINGER,
    MARKET_CLOSED,
    RATE_LIMIT,
    INVALID_TICK,
    INVALID_LOT
};
