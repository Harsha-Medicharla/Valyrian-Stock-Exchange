#pragma once
#include <cstdint>
#include "CoreTypes.h"
#include "../../ems/types/Common.h"

enum class OrderEventType : uint8_t
{
    FILL,
    REJECT,
    CANCEL_ACK,
    MODIFY_ACK,
};

struct alignas(64) OrderEvent
{
    OrderEventType type;
    uint8_t _pad0[7];
    OrderId order_id;
    UserId user_id;
    uint32_t symbol_id;
    uint32_t _pad1;
    Price fill_price;
    Qty fill_qty;
    Qty remaining;
    OrderState state;
    RejectReason reject_reason;
    uint8_t _pad2[6];
    SeqNo sequence;
    TimeStamp timestamp;
    uint8_t _pad3[48];
};

static_assert(sizeof(OrderEvent) == 128, "OrderEvent layout");

struct alignas(64) TradeEvent
{
    uint32_t symbol_id;
    Side aggressor_side;
    uint8_t _pad0[3];
    Price price;
    Qty qty;
    Price best_bid;
    Price best_ask;
    TimeStamp timestamp;
    uint8_t _pad1[16];
};

static_assert(sizeof(TradeEvent) == 64, "TradeEvent layout");

struct alignas(64) BookUpdateEvent
{
    uint32_t symbol_id;
    uint8_t _pad0[4];
    Price best_bid;
    Price best_ask;
    TimeStamp timestamp;
    uint8_t _pad1[32];
};

static_assert(sizeof(BookUpdateEvent) == 64, "BookUpdateEvent layout");

enum class DBEventType : uint8_t
{
    ORDER_ACCEPTED,
    ORDER_FILLED,
    ORDER_CANCELLED,
    ORDER_MODIFIED,
};

struct alignas(64) DBEvent
{
    SeqNo wal_sequence;
    DBEventType type;
    Side side;
    OrderType order_type;
    OrderState state;
    uint8_t _pad0[4];
    OrderId order_id;
    UserId user_id;
    uint32_t symbol_id;
    uint32_t _pad1;

    Price price;
    Qty qty;
    Qty remaining;
    Price fill_price;
    Qty fill_qty;
    TimeStamp timestamp;
    OrderId peer_order_id;
    UserId peer_user_id;
    uint8_t _pad2[24];
};

static_assert(sizeof(DBEvent) == 128, "DBEvent layout");
