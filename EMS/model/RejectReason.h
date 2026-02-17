#pragma once

#include <cstdint>

namespace EMS {
namespace model {

enum class RejectReason : uint8_t {
    NONE = 0,
    AUTH_FAILED,
    MARKET_CLOSED,
    RISK_REJECTED,
    SELF_TRADE_BLOCKED,
    SYSTEM_OVERLOAD
};

}
}
