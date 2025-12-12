#include <iostream>
#include <vector>
#include "OrderBook.h"
#include "wal/WAL.h"

class MatchingEngine
{
private:
    Symbol symbol;
    TimeStamp time_stamp;
    WALSystem wal;
    bool is_recovering = false;
    OrderBook order_book;

public:
    MatchingEngine(Symbol symbol) : symbol(symbol), time_stamp(0), wal("engine_" + std::to_string(symbol))
    {
        is_recovering = true;

        wal.recover([this](const LogEntry &entry)
                    {
            if (entry.action == WalAction::ADD) {
                Order* order = order_book.requestAllocationOfOrder();
                
                order->order_id = entry.data.order_id;
                order->user_id = entry.data.user_id;
                order->side = entry.data.side;
                order->type = entry.data.type;
                order->price = entry.data.price;
                order->quantity = entry.data.quantity;
                order->remaining = entry.data.remaining;
                order->timestamp = entry.data.timestamp;
                order->state = entry.data.state;
                
                this->onNewOrder(order);
            }
            else if (entry.action == WalAction::CANCEL) {
                this->onCancelOrder(entry.data.order_id);
            }
            else if (entry.action == WalAction::MODIFY) {
                this->onModifyOrder(entry.data.order_id, entry.data.price, entry.data.quantity);
            } });

        is_recovering = false;
    }

    ~MatchingEngine() = default;

    bool onNewOrder(Order *order)
    {
        if (!is_recovering)
        {
            wal.logInput(WalAction::ADD, order);
        }

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

            return true;
        }
        catch (const std::exception &e)
        {
            order_book.requestDeAllocationOfOrder(order);
            return false;
        }
    }

    bool onCancelOrder(OrderId order_id)
    {
        if (!is_recovering)
        {
            wal.logCancel(order_id);
        }

        try
        {
            Order *order = order_book.findOrder(order_id);
            if (!order || order->state == OrderState::FILLED)
            {
                return false;
            }
            order_book.removeOrder(order);
            return true;
        }
        catch (const std::exception &e)
        {
            return false;
        }
    }

    bool onModifyOrder(OrderId order_id, Price new_price, Qty new_qty)
    {
        if (!is_recovering)
            wal.logModify(order_id, new_price, new_qty);

        try
        {
            Order *order = order_book.findOrder(order_id);
            if (!order)
                return false;

            if (new_price == order->price and new_qty < order->quantity)
            {
                Qty reduction_qty = order->quantity - new_qty;
                if (order->remaining < reduction_qty)
                {
                    return false;
                }

                order->quantity -= reduction_qty;
                order->remaining -= reduction_qty;
                updateOrderState(order);

                if (order->state == OrderState::FILLED)
                {
                    order_book.removeOrder(order);
                }
                return true;
            }
            else
            {
                UserId user = order->user_id;
                Side side = order->side;
                OrderType type = order->type;
                onCancelOrder(order->order_id);

                Order *new_order = order_book.requestAllocationOfOrder();
                *new_order = {order_id, user, side, type, new_price, new_qty, new_qty, time_stamp++, OrderState::NEW, nullptr, nullptr};

                bool was_recovering = is_recovering;
                is_recovering = true;
                bool success = onNewOrder(new_order);
                is_recovering = was_recovering;
                return success;
            }
        }
        catch (const std::exception &e)
        {
            return false;
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

            executeTrade(incoming_order, resting_order, best_price, traded);

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
        if (!is_recovering)
        {
            wal.logTrade(aggressor->order_id, resting_order->order_id, price, qty);
        }
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
        else
        {
            order->state = OrderState::PARTIALLY_FILLED;
        }
    }
};

// int main()
// {
//     // To test, uncomment the following code block and make the order_book object public

//     std::cout << "Starting Engine" << std::endl;
//     MatchingEngine *engine = new MatchingEngine(1);
//     Order* bestSell = engine->order_book.getOrderAtBestPrice(Side::SELL);

//     if (bestSell != nullptr) {
//         std::cout << "\nSuccessful Restoration" << std::endl;
//         bool idMatch = (bestSell->order_id == 2);
//         bool qtyMatch = (bestSell->remaining == 30);
//         bool priceMatch = (bestSell->price == 110);
//         std::cout << "   -> Order ID: " << bestSell->order_id << (idMatch ? " [OK]" : " [FAIL]") << std::endl;
//         std::cout << "   -> Remaining: " << bestSell->remaining << (qtyMatch ? " [OK]" : " [FAIL]") << std::endl;
//         std::cout << "   -> Price: " << bestSell->price << (priceMatch ? " [OK]" : " [FAIL]") << std::endl;

//         if (idMatch && qtyMatch && priceMatch) {
//             std::cout << "\nSuccessful Restoration" << std::endl;
//         } else {
//             std::cout << "\nIncorrect data fetching" << std::endl;
//         }
//     }
//     else {
//         std::cout << "\nNo orders yet, filling orderBook" << std::endl;
//         Order* o1 = engine->order_book.requestAllocationOfOrder();
//         *o1 = {1, 101, Side::SELL, OrderType::LIMIT, 100, 50, 50, 0, OrderState::NEW, nullptr, nullptr};
//         engine->onNewOrder(o1);
//         std::cout << "1. Placed SELL 50 @ 100 (Order 1)" << std::endl;
//         Order* o2 = engine->order_book.requestAllocationOfOrder();
//         *o2 = {2, 101, Side::SELL, OrderType::LIMIT, 110, 50, 50, 0, OrderState::NEW, nullptr, nullptr};
//         engine->onNewOrder(o2);
//         std::cout << "2. Placed SELL 50 @ 110 (Order 2)" << std::endl;
//         Order* o3 = engine->order_book.requestAllocationOfOrder();
//         *o3 = {3, 101, Side::SELL, OrderType::LIMIT, 120, 50, 50, 0, OrderState::NEW, nullptr, nullptr};
//         engine->onNewOrder(o3);
//         std::cout << "3. Placed SELL 50 @ 120 (Order 3)" << std::endl;
//         Order* o4 = engine->order_book.requestAllocationOfOrder();
//         *o4 = {4, 102, Side::BUY, OrderType::MARKET, 0, 70, 70, 0, OrderState::NEW, nullptr, nullptr};
//         engine->onNewOrder(o4);
//         std::cout << "4. Placed BUY MARKET 70 (Sweeps Order 1 & Part of Order 2)" << std::endl;
//         engine->onCancelOrder(3);
//         std::cout << "5. Cancelled Order 3" << std::endl;

//         std::cout << "\nSequence Complete" << std::endl;
//         std::cout << "On rerunning, Order #2 should have 30 items left." << std::endl;
//     }

//     delete engine;
//     return 0;
// }
