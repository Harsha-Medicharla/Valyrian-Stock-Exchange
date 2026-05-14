#pragma once
#include <cstdint>

// Canonical type aliases shared by EMS and MatchingEngine.
// Both modules include this header. Neither defines these types locally.

using OrderId   = uint64_t;
using UserId    = uint64_t;
using Price     = int64_t;
using Qty       = int64_t;
using SeqNo     = uint64_t;
using TimeStamp = uint64_t;
using Symbol    = uint64_t;

enum class Side : uint8_t { BUY, SELL };
enum class OrderType : uint8_t { LIMIT, MARKET };
enum class OrderState : uint8_t { NEW, PARTIALLY_FILLED, FILLED, CANCELLED };

enum class WALEntryType : uint8_t
{
    WAL_NEW_ORDER,
    WAL_CANCEL_ORDER,
    WAL_MODIFY_ORDER,
    WAL_TRADE
};
