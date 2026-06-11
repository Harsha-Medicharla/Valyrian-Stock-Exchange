#pragma once

inline constexpr char kSqlOrdersUpsert[] =
    "INSERT INTO orders (order_id, user_id, symbol_id, price, qty, filled_qty, side, type, status, "
    "timestamp) VALUES ($1,$2,$3,$4,$5,0,$6,$7,0,$8) ON CONFLICT (order_id) DO NOTHING";

inline constexpr char kSqlOrdersUpdate[] =
    "UPDATE orders SET filled_qty = $2, status = $3, "
    "price = CASE WHEN price = 0 AND $4 > 0 THEN $4 ELSE price END "
    "WHERE order_id = $1";

inline constexpr char kSqlTradesInsert[] =
    "INSERT INTO trades (symbol_id, buyer_order_id, seller_order_id, buyer_user_id, "
    "seller_user_id, price, qty, timestamp) VALUES ($1,$2,$3,$4,$5,$6,$7,$8)";

inline constexpr char kSqlBalancesUpdate[] =
    "UPDATE balances SET available = available + $2, blocked = blocked - $3 WHERE user_id = $1";

inline constexpr char kSqlHoldingsUpsert[] =
    "INSERT INTO holdings (user_id, symbol_id, available_qty, blocked_qty) VALUES ($1,$2,$3,$4) ON "
    "CONFLICT (user_id, symbol_id) DO UPDATE SET available_qty = holdings.available_qty + "
    "EXCLUDED.available_qty, blocked_qty = holdings.blocked_qty + EXCLUDED.blocked_qty";
