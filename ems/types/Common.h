#pragma once
#include <cstdint>
#include "../../shared/types/CoreTypes.h"

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
    INVALID_LOT,
    INSUFFICIENT_FUNDS   // balance check failed
};
