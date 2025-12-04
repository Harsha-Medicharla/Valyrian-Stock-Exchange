#include "Types.h"

class Order
{
public:
    OrderId order_id{};
    UserId user_id{};
    Side side{};
    OrderType type{};
    Price price{};
    Qty quantity{};
    Qty remaining{};
    TimeStamp timestamp{};
    OrderState state{OrderState::NEW};
    Order *prev{nullptr};
    Order *next{nullptr};
    void reset()
    {
        *this = Order{};
    }
};
