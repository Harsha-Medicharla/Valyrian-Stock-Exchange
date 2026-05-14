#pragma once
#include <cstdint>
#include "CoreTypes.h"
#include "../../ems/types/Common.h"   // for RejectReason

// ── OrderEvent ───────────────────────────────────────────────────────────────
// Written by: MatchingEngine (fills, cancel acks, modify acks)
//             IngressWorker via RejectHandler callback (pre-trade rejects)
// Read by:    RespThread inside TradeServer
//
// One struct covers all outcomes so RespThread has a single drain loop.

enum class OrderEventType : uint8_t
{
    FILL,        // partial or full fill
    REJECT,      // pre-trade rejection from EMS
    CANCEL_ACK,  // cancel confirmed by engine
    MODIFY_ACK,  // modify confirmed by engine
};

struct alignas(64) OrderEvent
{
    OrderEventType type;
    uint8_t        _pad0[7];
    OrderId        order_id;
    UserId         user_id;
    uint32_t       symbol_id;
    uint32_t       _pad1;
    Price          fill_price;    // 0 for rejects, cancel acks, modify acks
    Qty            fill_qty;      // 0 for rejects, cancel acks, modify acks
    Qty            remaining;
    OrderState     state;
    RejectReason   reject_reason; // zero for non-rejects
    uint8_t        _pad2[6];
    SeqNo          sequence;      // server sequence for client correlation
    TimeStamp      timestamp;
    uint8_t        _pad3[48];     // pad struct to 128 bytes (two cache lines)
};

static_assert(sizeof(OrderEvent) == 128, "OrderEvent layout");

// ── TradeEvent ───────────────────────────────────────────────────────────────
// Written by: MatchingEngine after each matched trade
// Read by:    MarketDataPublisher

struct alignas(64) TradeEvent
{
    uint32_t  symbol_id;
    Side      aggressor_side;
    uint8_t   _pad0[3];
    Price     price;
    Qty       qty;
    Price     best_bid;   // best bid price after match; 0 if book is empty
    Price     best_ask;   // best ask price after match; 0 if book is empty
    TimeStamp timestamp;
    uint8_t   _pad1[16];
    // 4+1+3+8+8+8+8+8+16 = 64 bytes
};

static_assert(sizeof(TradeEvent) == 64, "TradeEvent layout");

// ── DBEvent ──────────────────────────────────────────────────────────────────
// Written by: MatchingEngine (fills, cancels, modifies) and
//             IngressWorker via RejectHandler (accepted orders — for orders table)
// Read by:    DBWriter
//
// wal_sequence is the WAL log entry number. Used as the idempotency key:
//   INSERT ... ON CONFLICT (wal_sequence) DO NOTHING
// Safe to replay after crash without double-counting.
//
// peer_order_id / peer_user_id are set for ORDER_FILLED (resting counterparty).

enum class DBEventType : uint8_t
{
    ORDER_ACCEPTED,  // new order accepted by EMS — write to orders table
    ORDER_FILLED,    // partial or full fill  — update orders, write trades, settle balances
    ORDER_CANCELLED, // cancel confirmed      — update orders, unblock holdings
    ORDER_MODIFIED,  // modify confirmed      — update orders
};

struct alignas(64) DBEvent
{
    // Cache line 1 — identity
    SeqNo       wal_sequence;
    DBEventType type;
    Side        side;
    OrderType   order_type;
    OrderState  state;
    uint8_t     _pad0[4];
    OrderId     order_id;
    UserId      user_id;
    uint32_t    symbol_id;
    uint32_t    _pad1;

    // Cache line 2 — quantities and prices
    Price     price;
    Qty       qty;
    Qty       remaining;
    Price     fill_price;   // 0 unless ORDER_FILLED
    Qty       fill_qty;     // 0 unless ORDER_FILLED
    TimeStamp timestamp;
    OrderId   peer_order_id;
    UserId    peer_user_id;
    uint8_t   _pad2[24];
};

static_assert(sizeof(DBEvent) == 128, "DBEvent layout");
