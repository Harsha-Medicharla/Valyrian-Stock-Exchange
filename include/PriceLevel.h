#include "Order.h"

class PriceLevel
{
public:
    Price price{};
    Qty aggregated_qty{0};
    Order *head{nullptr};
    Order *tail{nullptr};

    void fifoPush(Order *order) noexcept
    {
        order->next = nullptr;
        order->prev = this->tail;

        if (this->tail)
        {
            this->tail->next = order;
        }
        else
        {
            this->head = order;
        }

        this->tail = order;
        this->aggregated_qty += order->remaining;
    }

    void fifoRemove(Order *order) noexcept
    {
        if (order->prev)
        {
            order->prev->next = order->next;
        }
        else
        {
            this->head = order->next;
        }

        if (order->next)
        {
            order->next->prev = order->prev;
        }
        else
        {
            this->tail = order->prev;
        }

        this->aggregated_qty -= order->remaining;
        order->next = order->prev = nullptr;
    }

    bool empty() const noexcept
    {
        return this->head == nullptr;
    }

    void reset() noexcept
    {
        price = aggregated_qty = 0;
        head = tail = nullptr;
    }
};
