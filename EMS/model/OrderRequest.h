#pragma once

#include "../../MatchingEngine/include/Types.h"
#include <cstdint>

namespace EMS {
namespace model {

struct alignas(64) OrderRequest {
    OrderId   order_id;
    UserId    user_id;
    Symbol    symbol;

    Side      side;
    OrderType type;

    Price     price;
    Qty       quantity;

    uint64_t  wall_time_ns; 
    uint64_t  event_seq;
};

}
}
