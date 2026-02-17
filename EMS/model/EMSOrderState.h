#pragma once

#include <cstdint>

namespace EMS {
namespace model {

enum class EMSOrderState : uint8_t {
    RECEIVED = 0,
    VALIDATED,
    REJECTED,
    QUEUED,
    DISPATCHED
};

}
}
