#pragma once
#include "Pools.h"
#include "AdaptiveRadixTree.h"
#include <absl/container/flat_hash_map.h>

class OrderBook
{
private:
  OrderPool order_pool;
  PriceLevelPool price_level_pool;
  absl::flat_hash_map<OrderId, Order *> order_index;
  AdaptiveRadixTree buy_book;
  AdaptiveRadixTree sell_book;
  PriceLevel *best_bid;
  PriceLevel *best_ask;
  
  public:
  OrderBook() : best_bid(nullptr), best_ask(nullptr)
  {
    order_index.reserve(1 << 20);
  }

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

  Order *requestAllocationOfOrder()
  {
    return order_pool.allocate();
  }

  void requestDeAllocationOfOrder(Order *order)
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
        best_bid = buy_book.find(buy_book.maxPrice());
      }
      else
      {
        best_ask = sell_book.find(sell_book.minPrice());
      }
    }
    catch (...)
    {
      if (order->prev or order->next)
      {
        PriceLevel *level = getPriceLevel(order->side, order->price);
        if (level)
        {
          level->fifoRemove(order);
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
      if (buy_book.empty())
      {
        best_bid = nullptr;
      }
      else
      {
        best_bid = buy_book.find(buy_book.maxPrice());
      }
    }
    else
    {
      if (sell_book.empty())
      {
        best_ask = nullptr;
      }
      else
      {
        best_ask = sell_book.find(sell_book.minPrice());
      }
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

    PriceLevel *level = book.find(price);
    if (level != nullptr)
    {
      return level;
    }

    level = price_level_pool.allocate();

    try
    {
      level->price = price;
      book.insert(price, level);
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

    PriceLevel *level = book.find(price);
    if (level != nullptr)
    {
      return level;
    }
    else
    {
      return nullptr;
    }
  }

  void removePriceLevelIfEmpty(Side side, Price price) noexcept
  {
    auto &book = (side == Side::BUY) ? buy_book : sell_book;

    PriceLevel *level = book.find(price);
    if (level == nullptr)
    {
      return;
    }

    if (!level->empty())
    {
      return;
    }

    book.erase(level->price);
    price_level_pool.deallocate(level);
  }
};
