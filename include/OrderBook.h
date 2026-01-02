#include "Pools.h"
#include <map>
#include <unordered_map>

class OrderBook
{
private:
    OrderPool order_pool;
    PriceLevelPool price_level_pool;
    std::unordered_map<OrderId, Order *> order_index;
    std::map<Price, PriceLevel *> buy_book;
    std::map<Price, PriceLevel *> sell_book;
    PriceLevel *best_bid;
    PriceLevel *best_ask;

public:
    OrderBook() : best_bid(nullptr), best_ask(nullptr) {}

    Order *getOrderAtBestPrice(Side opposite_side) noexcept
    {
        PriceLevel *level = (opposite_side == Side::BUY) ? best_bid : best_ask;
        return (level) ? level->head : nullptr;
    }

    void consumeOrder(Order *order, Qty qty) noexcept
    {
        PriceLevel *level = getPriceLevel(order->side, order->price);
        order->remaining -= qty;
        level->aggregated_qty -= qty;
    }

    Order *requestAllocationOfOrder() noexcept
    {
        return order_pool.allocate();
    }

    void requestDeAllocationOfOrder(Order *order) noexcept
    {
        order_pool.deallocate(order);
    }

    void insertOrder(Order *order)
    {
        if (order == nullptr)
        {
            throw std::logic_error("insertOrder: null order");
        }
        try
        {
            PriceLevel *level = getOrCreatePriceLevel(order->side, order->price);

            level->fifoPush(order);

            order_index.emplace(order->order_id, order);

            if (order->side == Side::BUY)
            {
                best_bid = buy_book.rbegin()->second;
            }
            else
            {
                best_ask = sell_book.begin()->second;
            }
        }
        catch (...)
        {
            if (order->prev or order->next)
            {
                PriceLevel *lvl = getPriceLevel(order->side, order->price);
                if (lvl)
                {
                    lvl->fifoRemove(order);
                }
            }
            throw;
        }
    }

    void removeOrder(Order *order)
    {
        if (order == nullptr)
        {
            throw std::logic_error("removeOrder: null order");
        }

        PriceLevel *level = getPriceLevel(order->side, order->price);
        if (!level)
        {
            throw std::logic_error("removeOrder: price level missing");
        }

        level->fifoRemove(order);

        order_index.erase(order->order_id);

        removePriceLevelIfEmpty(order->side, order->price);

        if (order->side == Side::BUY)
        {
            best_bid = buy_book.empty() ? nullptr : buy_book.rbegin()->second;
        }
        else
        {
            best_ask = sell_book.empty() ? nullptr : sell_book.begin()->second;
        }

        order_pool.deallocate(order);
    }

    Order *findOrder(OrderId order_id) noexcept
    {
        auto it = order_index.find(order_id);
        if (it == order_index.end())
        {
            return nullptr;
        }
        return it->second;
    }

private:
    PriceLevel *getOrCreatePriceLevel(Side side, Price price)
    {
        auto &book = (side == Side::BUY) ? buy_book : sell_book;

        auto it = book.find(price);
        if (it != book.end())
        {
            return it->second;
        }

        PriceLevel *level = price_level_pool.allocate();

        try
        {
            level->price = price;
            book.emplace(price, level);
        }
        catch (...)
        {
            price_level_pool.deallocate(level);
            throw;
        }

        return level;
    }

    PriceLevel *getPriceLevel(Side side, Price price) noexcept
    {
        auto &book = (side == Side::BUY) ? buy_book : sell_book;

        auto lvl_it = book.find(price);
        if (lvl_it != book.end())
        {
            return lvl_it->second;
        }
        else
        {
            return nullptr;
        }
    }

    void removePriceLevelIfEmpty(Side side, Price price) noexcept
    {
        auto &book = (side == Side::BUY) ? buy_book : sell_book;

        auto it = book.find(price);
        if (it == book.end())
        {
            return;
        }

        PriceLevel *level = it->second;

        if (!level->empty())
        {
            return;
        }

        book.erase(it);
        price_level_pool.deallocate(level);
    }
};
