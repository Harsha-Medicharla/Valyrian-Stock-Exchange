#pragma once
#include <cstdint>

namespace EMS {
namespace model {

// enum class RejectReason : uint8_t {
//     NONE = 0,
//     AUTH_FAILED,
//     MARKET_CLOSED,
//     RISK_EXCEEDED,
//     SELF_TRADE_BLOCKED,
//     SYSTEM_OVERLOAD,
//     INSUFFICIENT_FUNDS,
//     INSUFFICIENT_SHARES
// };
enum class RejectReason : uint8_t {
    NONE = 0,
    AUTH_FAILED = 1,
    RISK_EXCEEDED = 2,
    MARKET_CLOSED = 3,
    INSUFFICIENT_FUNDS = 4,
    INSUFFICIENT_SHARES = 5  // Ensure this exists!
};
}
}