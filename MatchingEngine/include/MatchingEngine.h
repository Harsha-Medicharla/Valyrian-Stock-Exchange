#pragma once
#include <iostream>
#include "OrderBook.h"

class MatchingEngine
{
private:
    Symbol symbol;
    OrderBook order_book;
    TimeStamp time_stamp;
    // WAL* wal;
public:
    MatchingEngine(Symbol symbol) : symbol(symbol), time_stamp(0) {}

    void onNewOrder(Order *order)
    {
        try
        {
            match(order);
            updateOrderState(order);

            if (order->type == OrderType::MARKET or order->state == OrderState::FILLED)
            {
                order_book.requestDeAllocationOfOrder(order);
            }
            else
            {
                order_book.insertOrder(order);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "onNewOrder failed: " << e.what() << "\n";
            order_book.requestDeAllocationOfOrder(order);
        }
    }

    void onCancelOrder(OrderId order_id)
    {
        try
        {
            Order *order = order_book.findOrder(order_id);
            if (!order)
            {
                return;
            }

            if (order->state == OrderState::FILLED)
            {
                return;
            }

            order_book.removeOrder(order);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Cancel failed: " << e.what() << "\n";
        }
    }

    void onModifyOrder(OrderId order_id, Price new_price, Qty new_qty)
    {
        try
        {
            Order *order = order_book.findOrder(order_id);
            if (!order)
            {
                return;
            }

            if (new_price == order->price and new_qty < order->quantity)
            {
                Qty reduction_qty = order->quantity - new_qty;
                if (order->remaining < reduction_qty)
                {
                    throw std::logic_error("Invalid modify quantity");
                }

                order->quantity -= reduction_qty;
                order->remaining -= reduction_qty;
                updateOrderState(order);

                if (order->state == OrderState::FILLED)
                {
                    order_book.removeOrder(order);
                }
            }
            else
            {
                UserId user_id = order->user_id;
                Side side = order->side;
                OrderType type = order->type;

                onCancelOrder(order_id);

                Order *new_order = order_book.requestAllocationOfOrder();
                *new_order = {order_id, user_id, side, type, new_price, new_qty, new_qty, time_stamp++, OrderState::NEW, nullptr, nullptr};

                onNewOrder(new_order);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Modify failed: " << e.what() << "\n";
        }
    }

private:
    void match(Order *incoming_order) noexcept
    {
        Side opposite_side = (incoming_order->side == Side::BUY) ? Side::SELL : Side::BUY;
        while (incoming_order->remaining > 0)
        {
            Order *resting_order = order_book.getOrderAtBestPrice(opposite_side);
            if (!resting_order)
            {
                break;
            }
            Price best_price = resting_order->price;
            if (incoming_order->type == OrderType::LIMIT)
            {
                if (incoming_order->side == Side::BUY && incoming_order->price < best_price)
                {
                    break;
                }
                if (incoming_order->side == Side::SELL && incoming_order->price > best_price)
                {
                    break;
                }
            }
            Qty traded = std::min(incoming_order->remaining, resting_order->remaining);

            incoming_order->remaining -= traded;
            order_book.consumeOrder(resting_order, traded);

            updateOrderState(resting_order);

            if (resting_order->state == OrderState::FILLED)
            {
                order_book.removeOrder(resting_order);
            }
        }
    }

    void executeTrade(Order *aggressor, Order *resting_order, Price price, Qty qty)
    {
        // related to WAL
        /*
        NOTE : the attributes related to this method is not declared
        in any of the currently implemented files, make sure to do so while
        implementing WAL
        */
        /*
        and also the necessary methods of WAL that are required to be called at
        required places of matching engine are not done, make sure to do so
        */
    }

    void updateOrderState(Order *order) noexcept
    {
        if (order->remaining == order->quantity)
        {
            order->state = OrderState::NEW;
        }
        else if (order->remaining == 0)
        {
            order->state = OrderState::FILLED;
        }
        else if (order->remaining > 0 and order->remaining < order->quantity)
        {
            order->state = OrderState::PARTIALLY_FILLED;
        }
    }
};
