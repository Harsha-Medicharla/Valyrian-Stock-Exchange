#pragma once

#include "OrderBook.h"
#include "wal/WAL.h"
#include "Settlement/core/Settlement.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

class MatchingEngine {
private:
    Symbol symbol; // Numeric ID (uint64_t)
    TimeStamp time_stamp;
    WALSystem wal;
    bool is_recovering = false;
    SettlementCore::Settlement& settlement; 

public:
    OrderBook order_book;
    
    MatchingEngine(Symbol symbol, SettlementCore::Settlement& settlement_module)
        : symbol(symbol), time_stamp(0), wal("engine_" + std::to_string(symbol)), settlement(settlement_module)
    {
        is_recovering = true;
        wal.recover([this](const LogEntry &entry) {
            if (entry.action == WalAction::ADD) {
                this->onNewOrder(entry.data.order_id, entry.data.user_id, entry.data.side, 
                                 entry.data.type, entry.data.price, entry.data.quantity, 
                                 entry.data.timestamp);
            } else if (entry.action == WalAction::CANCEL) {
                this->onCancelOrder(entry.data.order_id);
            } else if (entry.action == WalAction::MODIFY) {
                this->onModifyOrder(entry.data.order_id, entry.data.price, entry.data.quantity);
            } 
        });
        is_recovering = false;
    }

    ~MatchingEngine() = default;

    bool onNewOrder(OrderId order_id, UserId user_id, Side side, OrderType type, Price price, Qty qty, TimeStamp timestamp) {
        Order *order = order_book.requestAllocationOfOrder();
        if (!order) return false;
        *order = {order_id, user_id, side, type, price, qty, qty, timestamp, OrderState::NEW, nullptr, nullptr};
        return onNewOrder(order);
    }

    bool onNewOrder(Order *order) {
        if (!is_recovering) {
            try { wal.logInput(WalAction::ADD, order); }
            catch (...) { order_book.requestDeAllocationOfOrder(order); return false; }
        }
        match(order);
        updateOrderState(order);
        if (order->type == OrderType::MARKET || order->state == OrderState::FILLED || order->state == OrderState::CANCELLED) {
            order_book.requestDeAllocationOfOrder(order);
        } else {
            order_book.insertOrder(order);
        }
        return true;
    }

    bool onCancelOrder(OrderId order_id) {
        Order *order = order_book.findOrder(order_id);
        if (!order || order->state == OrderState::FILLED) return false;
        if (!is_recovering) wal.logCancel(order_id);
        
        // FIXED: symbol passed as uint64_t, no string conversion
        settlement.releaseMargin(order->user_id, symbol, static_cast<uint8_t>(order->side), order->price, order->remaining);
        
        order_book.removeOrder(order);
        return true;
    }

    bool onModifyOrder(OrderId order_id, Price new_price, Qty new_qty) {
        Order *order = order_book.findOrder(order_id);
        if (!order) return false;
        if (!is_recovering) wal.logModify(order_id, new_price, new_qty);
        
        onCancelOrder(order_id);
        return onNewOrder(order_id, order->user_id, order->side, order->type, new_price, new_qty, 0);
    }

    void match(Order *incoming_order) noexcept {
        Side opposite_side = (incoming_order->side == Side::BUY) ? Side::SELL : Side::BUY;
        
        while (incoming_order->remaining > 0) {
            Order *resting_order = order_book.getOrderAtBestPrice(opposite_side);
            if (!resting_order) break;
            
            if (incoming_order->type == OrderType::LIMIT) {
                if (incoming_order->side == Side::BUY && incoming_order->price < resting_order->price) break;
                if (incoming_order->side == Side::SELL && incoming_order->price > resting_order->price) break;
            }

            // Self-match prevention
            if (incoming_order->user_id == resting_order->user_id) {
                settlement.releaseMargin(incoming_order->user_id, symbol, static_cast<uint8_t>(incoming_order->side), incoming_order->price, incoming_order->remaining);
                incoming_order->remaining = 0;
                incoming_order->state = OrderState::CANCELLED;
                break;
            }

            Qty traded = std::min(incoming_order->remaining, resting_order->remaining);
            executeTrade(incoming_order, resting_order, resting_order->price, traded);

            incoming_order->remaining -= traded;
            order_book.consumeOrder(resting_order, traded);
            updateOrderState(resting_order);

            if (resting_order->state == OrderState::FILLED) {
                order_book.removeOrder(resting_order);
            }
        }
    }

    void executeTrade(Order *aggressor, Order *resting_order, Price price, Qty qty) {
        if (!is_recovering) wal.logTrade(aggressor->order_id, resting_order->order_id, price, qty);
        
        uint64_t buyer_id = (aggressor->side == Side::BUY) ? aggressor->user_id : resting_order->user_id;
        uint64_t seller_id = (aggressor->side == Side::SELL) ? aggressor->user_id : resting_order->user_id;
        
        // FIXED: symbol passed as uint64_t
        settlement.settleTrade(buyer_id, seller_id, symbol, price, qty);
    }

    void updateOrderState(Order *order) noexcept {
        if (order->remaining == order->quantity) order->state = OrderState::NEW;
        else if (order->remaining == 0) order->state = OrderState::FILLED;
        else order->state = OrderState::PARTIALLY_FILLED;
    }
};