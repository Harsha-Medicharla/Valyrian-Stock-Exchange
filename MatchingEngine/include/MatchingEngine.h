#include "OrderBook.h"
#include "wal/WAL.h"
#include <iostream>
#include <vector>

class MatchingEngine
{
private:
  Symbol symbol;
  TimeStamp time_stamp;
  WALSystem wal;
  bool is_recovering = false;
  OrderBook order_book;

public:
  MatchingEngine(Symbol symbol)
      : symbol(symbol), time_stamp(0), wal("engine_" + std::to_string(symbol))
  {
    is_recovering = true;

    wal.recover([this](const LogEntry &entry)
                {
      if (entry.action == WalAction::ADD) {
        Order *order = order_book.requestAllocationOfOrder();

        order->order_id = entry.data.order_id;
        order->user_id = entry.data.user_id;
        order->side = entry.data.side;
        order->type = entry.data.type;
        order->price = entry.data.price;
        order->quantity = entry.data.quantity;
        order->remaining = entry.data.remaining;
        order->timestamp = entry.data.timestamp;
        order->state = entry.data.state;

        this->onNewOrder(order->order_id,order->user_id,order->side,order->type,order->price,order->quantity,order->timestamp);
      } else if (entry.action == WalAction::CANCEL) {
        this->onCancelOrder(entry.data.order_id);
      } else if (entry.action == WalAction::MODIFY) {
        this->onModifyOrder(entry.data.order_id, entry.data.price,
                            entry.data.quantity);
      } });

    is_recovering = false;
  }

  ~MatchingEngine() = default;

  bool onNewOrder(OrderId order_id, UserId user_id, Side side, OrderType type, Price price, Qty qty, TimeStamp timestamp)
  {
    Order *order;
    try
    {
      order = order_book.requestAllocationOfOrder();
      *order = {order_id, user_id, side, type, price, qty, qty, timestamp, OrderState::NEW, nullptr, nullptr};
    }
    catch (const std::exception &e)
    {
      return false;
    }
    if (!is_recovering)
    {
      try
      {
        wal.logInput(WalAction::ADD, order);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in onNewOrder: " << e.what()
                  << std::endl;
        order_book.requestDeAllocationOfOrder(order);
        return false;
      }
    }

    try
    {
      match(order);
      updateOrderState(order);

      if (order->type == OrderType::MARKET or
          order->state == OrderState::FILLED or
          order->state == OrderState::CANCELLED)
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
      std::cerr << "[MatchingEngine] Logic Error in onNewOrder: " << e.what()
                << std::endl;
      order_book.requestDeAllocationOfOrder(order);
      return false;
    }
  }

  bool onCancelOrder(OrderId order_id)
  {
    if (!is_recovering)
    {
      try
      {
        wal.logCancel(order_id);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in onCancelOrder: " << e.what()
                  << std::endl;
        return false;
      }
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
    {
      try
      {
        wal.logModify(order_id, new_price, new_qty);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in onModifyOrder: " << e.what()
                  << std::endl;
        return false;
      }
    }

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

        *new_order = {order_id, user, side, type,
                      new_price, new_qty, new_qty, getCurrentWallTime(),
                      OrderState::NEW, nullptr, nullptr};
        bool was_recovering = is_recovering;
        is_recovering = true;
        bool success = onNewOrder(new_order->order_id, new_order->user_id, new_order->side, new_order->type, new_order->price, new_order->quantity, new_order->timestamp);
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
    Side opposite_side =
        (incoming_order->side == Side::BUY) ? Side::SELL : Side::BUY;
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
        if (incoming_order->side == Side::BUY &&
            incoming_order->price < best_price)
        {
          break;
        }
        if (incoming_order->side == Side::SELL &&
            incoming_order->price > best_price)
        {
          break;
        }
      }

      if (incoming_order->user_id == resting_order->user_id)
      {
        if (!is_recovering)
        {
          try
          {
            wal.logCancel(incoming_order->order_id);
          }
          catch (const std::exception &e)
          {
            std::cerr << "[MatchingEngine] WAL Error in onCancelOrder: " << e.what()
                      << std::endl;
          }
        }
        incoming_order->remaining = 0;
        incoming_order->state = OrderState::CANCELLED;
        break;
      }

      Qty traded =
          std::min(incoming_order->remaining, resting_order->remaining);

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

  void executeTrade(Order *aggressor, Order *resting_order, Price price,
                    Qty qty)
  {
    if (!is_recovering)
    {
      try
      {
        wal.logTrade(aggressor->order_id, resting_order->order_id, price, qty);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in executeTrade: " << e.what()
                  << std::endl;
      }
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
