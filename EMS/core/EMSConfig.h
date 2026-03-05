#pragma once
#include <cstdint>

namespace EMS {

struct EMSConfig {
    static constexpr uint64_t FAT_FINGER_LIMIT = 1000000;
    static constexpr uint64_t MAX_ORDERS_PER_SEC = 10000;
    static constexpr bool REJECT_ON_MARKET_CLOSE = true;
};

}