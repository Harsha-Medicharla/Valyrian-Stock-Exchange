#pragma once

#include <iostream>
#include <vector>

#include "OrderBook.h"
#include "shared/types/Events.h"
#include "shared/queues/EventSPSC.h"
#include "wal/WAL.h"

namespace vse::test
{
struct MatchingEnginePeer;
}

class MatchingEngine
{
  friend struct vse::test::MatchingEnginePeer;

private:
  OrderBook order_book_;
  Symbol symbol;
  TimeStamp time_stamp;
  WALSystem wal;
  bool isRecovering_{false};

  EventSPSC<OrderEvent> *orderQueue_{nullptr};
  EventSPSC<TradeEvent> *tradeQueue_{nullptr};
  EventSPSC<DBEvent> *dbQueue_{nullptr};
  uint32_t symbolId_{0};

  void match(Order *incoming_order, TimeStamp action_ts) noexcept;

  void emitPostTradeEvents(Order *incoming_order, const Order &resting_snap, Price trade_price,
                           Qty traded, TimeStamp action_ts) noexcept;

  void pushFillOrderEvent(const Order *o, Price fill_price, Qty fill_qty,
                          Qty remaining_after, OrderState st, TimeStamp action_ts) noexcept;

  void pushCancelAck(OrderId oid, UserId uid, OrderState st, Qty rem, TimeStamp action_ts) noexcept;

  void pushModifyAck(OrderId oid, UserId uid, OrderType ty, Side sd, Price px, Qty qty,
                     Qty rem, OrderState st, TimeStamp action_ts) noexcept;

  void pushDbCancel(OrderId oid, UserId uid, Side sd, OrderType ty, Price px, Qty qty,
                    Qty rem, TimeStamp action_ts) noexcept;

  void pushDbModify(OrderId oid, UserId uid, Side sd, OrderType ty, Price px, Qty qty,
                    Qty rem, OrderState st, TimeStamp action_ts) noexcept;

public:
  MatchingEngine(Symbol sym)
      : order_book_(),
        symbol(sym),
        time_stamp(0),
        wal("engine_" + std::to_string(sym)),
        isRecovering_(false)
  {
    isRecovering_ = true;

    wal.recover([this](const LogEntry &entry) {
      if (entry.action == WalAction::ADD)
      {
        this->onNewOrder(entry.data.order_id, entry.data.user_id, entry.data.side,
                         entry.data.type, entry.data.price, entry.data.quantity,
                         entry.data.timestamp);
      }
      else if (entry.action == WalAction::CANCEL)
      {
        this->onCancelOrder(entry.data.order_id);
      }
      else if (entry.action == WalAction::MODIFY)
      {
        this->onModifyOrder(entry.data.order_id, entry.data.price, entry.data.quantity);
      }
      else if (entry.action == WalAction::TRADE)
      {
        // Trade log records advance WAL sequence numbers but are not replayed into the book.
      }
    });

    isRecovering_ = false;
  }

  ~MatchingEngine() = default;

  void setOutputQueues(uint32_t symbolId, EventSPSC<OrderEvent> &orderQ, EventSPSC<TradeEvent> &tradeQ,
                       EventSPSC<DBEvent> &dbQ) noexcept
  {
    symbolId_ = symbolId;
    orderQueue_ = &orderQ;
    tradeQueue_ = &tradeQ;
    dbQueue_ = &dbQ;
  }

  bool onNewOrder(OrderId order_id, UserId user_id, Side side, OrderType type, Price price, Qty qty,
                  TimeStamp timestamp)
  {
    Order *order;
    try
    {
      order = order_book_.requestAllocationOfOrder();
      *order = {order_id, user_id, side, type, price, qty, qty, timestamp, OrderState::NEW, nullptr,
                nullptr};
    }
    catch (const std::exception &e)
    {
      return false;
    }
    try
    {
      const TimeStamp action_ts = timestamp != 0 ? timestamp : getCurrentWallTime();
      order->timestamp = action_ts;
      match(order, action_ts);
      updateOrderState(order);

      if (order->type == OrderType::MARKET && order->remaining > 0)
      {
        order->state = OrderState::CANCELLED;
        pushCancelAck(order->order_id, order->user_id, order->state, order->remaining, action_ts);
      }

      if (order->type == OrderType::MARKET || order->state == OrderState::FILLED ||
          order->state == OrderState::CANCELLED)
      {
        if (!isRecovering_)
        {
          try
          {
            wal.logInput(WalAction::ADD, order);
          }
          catch (const std::exception &e)
          {
            std::cerr << "[MatchingEngine] WAL Error in onNewOrder: " << e.what() << std::endl;
            order_book_.requestDeAllocationOfOrder(order);
            return false;
          }
        }
        order_book_.requestDeAllocationOfOrder(order);
      }
      else
      {
        order_book_.insertOrder(order);
        if (!isRecovering_)
        {
          try
          {
            wal.logInput(WalAction::ADD, order);
          }
          catch (const std::exception &e)
          {
            std::cerr << "[MatchingEngine] WAL Error in onNewOrder: " << e.what() << std::endl;
            order_book_.removeOrder(order);
            return false;
          }
        }
      }

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << "[MatchingEngine] Logic Error in onNewOrder: " << e.what() << std::endl;
      order_book_.requestDeAllocationOfOrder(order);
      return false;
    }
  }

  bool onCancelOrder(OrderId order_id)
  {
    return onCancelOrder(order_id, getCurrentWallTime());
  }

  bool onCancelOrder(OrderId order_id, TimeStamp action_ts) noexcept
  {
    if (!isRecovering_)
    {
      try
      {
        wal.logCancel(order_id);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in onCancelOrder: " << e.what() << std::endl;
        return false;
      }
    }

    try
    {
      Order *order = order_book_.findOrder(order_id);
      if (!order || order->state == OrderState::FILLED)
      {
        return false;
      }

      const UserId uid = order->user_id;
      const Side sd = order->side;
      const OrderType ty = order->type;
      const Price px = order->price;
      const Qty q = order->quantity;
      const Qty rem = order->remaining;
      const OrderState st = order->state;

      order_book_.removeOrder(order);

      pushCancelAck(order_id, uid, st, rem, action_ts);
      pushDbCancel(order_id, uid, sd, ty, px, q, rem, action_ts);

      return true;
    }
    catch (const std::exception &e)
    {
      return false;
    }
  }

  bool onModifyOrder(OrderId order_id, Price new_price, Qty new_qty)
  {
    const TimeStamp action_ts = getCurrentWallTime();
    try
    {
      Order *order = order_book_.findOrder(order_id);
      if (!order)
        return false;

      if (new_price == order->price && new_qty < order->quantity)
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
          order_book_.removeOrder(order);
        }
        else
        {
          pushModifyAck(order_id, order->user_id, order->type, order->side, order->price,
                        order->quantity, order->remaining, order->state, action_ts);
          pushDbModify(order_id, order->user_id, order->side, order->type, order->price,
                       order->quantity, order->remaining, order->state, action_ts);
        }
        return true;
      }
      else
      {
        UserId user = order->user_id;
        Side side = order->side;
        OrderType type = order->type;

        if (!onCancelOrder(order->order_id, action_ts))
        {
          return false;
        }
        bool success = onNewOrder(order_id, user, side, type, new_price, new_qty, action_ts);
        if (success)
        {
          Order *placed = order_book_.findOrder(order_id);
          if (placed)
          {
            pushModifyAck(order_id, placed->user_id, placed->type, placed->side, placed->price,
                          placed->quantity, placed->remaining, placed->state, action_ts);
            pushDbModify(order_id, placed->user_id, placed->side, placed->type, placed->price,
                         placed->quantity, placed->remaining, placed->state, action_ts);
          }
        }
        return success;
      }
    }
    catch (const std::exception &e)
    {
      return false;
    }
  }

private:
  void executeTrade(Order *aggressor, Order *resting_order, Price price, Qty qty)
  {
    if (!isRecovering_)
    {
      try
      {
        wal.logTrade(aggressor->order_id, resting_order->order_id, price, qty);
      }
      catch (const std::exception &e)
      {
        std::cerr << "[MatchingEngine] WAL Error in executeTrade: " << e.what() << std::endl;
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

inline void MatchingEngine::pushFillOrderEvent(const Order *o, Price fill_price, Qty fill_qty,
                                               Qty remaining_after, OrderState st,
                                               TimeStamp action_ts) noexcept
{
  if (!orderQueue_ || isRecovering_)
    return;
  OrderEvent ev{};
  ev.type = OrderEventType::FILL;
  ev.order_id = o->order_id;
  ev.user_id = o->user_id;
  ev.symbol_id = symbolId_;
  ev.fill_price = fill_price;
  ev.fill_qty = fill_qty;
  ev.remaining = remaining_after;
  ev.state = st;
  ev.reject_reason = static_cast<RejectReason>(0);
  ev.sequence = 0;
  ev.timestamp = action_ts;
  (void)orderQueue_->tryPush(ev);
}

inline void MatchingEngine::emitPostTradeEvents(Order *incoming_order, const Order &resting_snap,
                                                Price trade_price, Qty traded, TimeStamp action_ts) noexcept
{
  const SeqNo walSeq = static_cast<SeqNo>(wal.lastSequence());

  if (dbQueue_ && !isRecovering_)
  {
    DBEvent da{};
    da.wal_sequence = walSeq;
    da.type = DBEventType::ORDER_FILLED;
    da.side = incoming_order->side;
    da.order_type = incoming_order->type;
    da.order_id = incoming_order->order_id;
    da.user_id = incoming_order->user_id;
    da.symbol_id = symbolId_;
    da.price = incoming_order->price;
    da.qty = incoming_order->quantity;
    da.remaining = incoming_order->remaining;
    da.fill_price = trade_price;
    da.fill_qty = traded;
    da.timestamp = action_ts;
    da.peer_order_id = resting_snap.order_id;
    da.peer_user_id = resting_snap.user_id;
    da.state = incoming_order->state;
    (void)dbQueue_->tryPush(da);

    DBEvent dr{};
    dr.wal_sequence = walSeq;
    dr.type = DBEventType::ORDER_FILLED;
    dr.side = resting_snap.side;
    dr.order_type = resting_snap.type;
    dr.order_id = resting_snap.order_id;
    dr.user_id = resting_snap.user_id;
    dr.symbol_id = symbolId_;
    dr.price = resting_snap.price;
    dr.qty = resting_snap.quantity;
    dr.remaining = resting_snap.remaining;
    dr.fill_price = trade_price;
    dr.fill_qty = traded;
    dr.timestamp = action_ts;
    dr.peer_order_id = incoming_order->order_id;
    dr.peer_user_id = incoming_order->user_id;
    dr.state = resting_snap.state;
    (void)dbQueue_->tryPush(dr);
  }

  if (orderQueue_ && !isRecovering_)
  {
    pushFillOrderEvent(incoming_order, trade_price, traded, incoming_order->remaining,
                       incoming_order->state, action_ts);
    pushFillOrderEvent(&resting_snap, trade_price, traded, resting_snap.remaining, resting_snap.state,
                       action_ts);
  }

  if (tradeQueue_ && !isRecovering_)
  {
    TradeEvent te{};
    te.symbol_id = symbolId_;
    te.aggressor_side = incoming_order->side;
    te.price = static_cast<Price>(trade_price);
    te.qty = static_cast<Qty>(traded);
    te.best_bid = order_book_.bestBidPrice();
    te.best_ask = order_book_.bestAskPrice();
    te.timestamp = action_ts;
    (void)tradeQueue_->tryPush(te);
  }
}

inline void MatchingEngine::pushCancelAck(OrderId oid, UserId uid, OrderState st,
                                          Qty rem, TimeStamp action_ts) noexcept
{
  if (!orderQueue_ || isRecovering_)
    return;
  OrderEvent ev{};
  ev.type = OrderEventType::CANCEL_ACK;
  ev.order_id = oid;
  ev.user_id = uid;
  ev.symbol_id = symbolId_;
  ev.remaining = rem;
  ev.state = st;
  ev.timestamp = action_ts;
  (void)orderQueue_->tryPush(ev);
}

inline void MatchingEngine::pushModifyAck(OrderId oid, UserId uid, OrderType ty, Side sd, Price px,
                                          Qty qty, Qty rem, OrderState st, TimeStamp action_ts) noexcept
{
  if (!orderQueue_ || isRecovering_)
    return;
  OrderEvent ev{};
  ev.type = OrderEventType::MODIFY_ACK;
  ev.order_id = oid;
  ev.user_id = uid;
  ev.symbol_id = symbolId_;
  ev.remaining = rem;
  ev.state = st;
  ev.timestamp = action_ts;
  (void)orderQueue_->tryPush(ev);
  (void)ty;
  (void)sd;
  (void)px;
  (void)qty;
}

inline void MatchingEngine::pushDbCancel(OrderId oid, UserId uid, Side sd, OrderType ty, Price px,
                                         Qty qty, Qty rem, TimeStamp action_ts) noexcept
{
  if (!dbQueue_ || isRecovering_)
    return;
  DBEvent de{};
  de.wal_sequence = static_cast<SeqNo>(wal.lastSequence());
  de.type = DBEventType::ORDER_CANCELLED;
  de.side = sd;
  de.order_type = ty;
  de.state = OrderState::CANCELLED;
  de.order_id = oid;
  de.user_id = uid;
  de.symbol_id = symbolId_;
  de.price = px;
  de.qty = qty;
  de.remaining = rem;
  de.timestamp = action_ts;
  (void)dbQueue_->tryPush(de);
}

inline void MatchingEngine::pushDbModify(OrderId oid, UserId uid, Side sd, OrderType ty, Price px,
                                         Qty qty, Qty rem, OrderState st, TimeStamp action_ts) noexcept
{
  if (!dbQueue_ || isRecovering_)
    return;
  DBEvent de{};
  de.wal_sequence = static_cast<SeqNo>(wal.lastSequence());
  de.type = DBEventType::ORDER_MODIFIED;
  de.side = sd;
  de.order_type = ty;
  de.state = st;
  de.order_id = oid;
  de.user_id = uid;
  de.symbol_id = symbolId_;
  de.price = px;
  de.qty = qty;
  de.remaining = rem;
  de.timestamp = action_ts;
  (void)dbQueue_->tryPush(de);
}

inline void MatchingEngine::match(Order *incoming_order, TimeStamp action_ts) noexcept
{
  Side opposite_side = (incoming_order->side == Side::BUY) ? Side::SELL : Side::BUY;
  while (incoming_order->remaining > 0)
  {
    Order *resting_order = order_book_.getOrderAtBestPrice(opposite_side);
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

    if (incoming_order->user_id == resting_order->user_id)
    {
      if (!isRecovering_)
      {
        try
        {
          wal.logCancel(resting_order->order_id);
        }
        catch (const std::exception &e)
        {
          std::cerr << "[MatchingEngine] WAL Error in onNewOrder self-trade: " << e.what()
                    << std::endl;
        }
      }
      const OrderId resting_id = resting_order->order_id;
      const UserId resting_uid = resting_order->user_id;
      const Side resting_side = resting_order->side;
      const OrderType resting_type = resting_order->type;
      const Price resting_price = resting_order->price;
      const Qty resting_qty = resting_order->quantity;
      const Qty resting_remaining = resting_order->remaining;

      order_book_.removeOrder(resting_order);
      pushCancelAck(resting_id, resting_uid, OrderState::CANCELLED, resting_remaining, action_ts);
      pushDbCancel(resting_id, resting_uid, resting_side, resting_type, resting_price, resting_qty,
                   resting_remaining, action_ts);
      continue;
    }

    Qty traded = std::min(incoming_order->remaining, resting_order->remaining);

    executeTrade(incoming_order, resting_order, best_price, traded);

    incoming_order->remaining -= traded;
    order_book_.consumeOrder(resting_order, traded);
    updateOrderState(resting_order);
    updateOrderState(incoming_order);

    const Order resting_snap = *resting_order;

    if (resting_order->state == OrderState::FILLED)
    {
      order_book_.removeOrder(resting_order);
    }

    emitPostTradeEvents(incoming_order, resting_snap, best_price, traded, action_ts);
  }
}
