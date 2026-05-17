#include "PGWriter.h"

#include <cstdio>
#include <string>
#include <utility>

#include <libpq-fe.h>

#include "Schema.h"

PGWriter::PGWriter(std::string connString) : connString_(std::move(connString))
{
    conn_ = PQconnectdb(connString_.c_str());
    if (!conn_ || PQstatus(conn_) != CONNECTION_OK)
    {
        if (conn_)
            PQfinish(conn_);
        conn_ = nullptr;
    }
}

PGWriter::~PGWriter()
{
    if (conn_)
        PQfinish(conn_);
}

void PGWriter::writeBatch(const std::vector<DBEvent> &batch)
{
    // Reconnect if connection is missing or broken
    if (!conn_ || PQstatus(conn_) != CONNECTION_OK)
    {
        if (conn_)
        {
            PQreset(conn_);
            if (PQstatus(conn_) != CONNECTION_OK)
            {
                PQfinish(conn_);
                conn_ = nullptr;
            }
        }
        if (!conn_)
        {
            conn_ = PQconnectdb(connString_.c_str());
            if (!conn_ || PQstatus(conn_) != CONNECTION_OK)
            {
                std::fprintf(stderr,
                    "PGWriter: reconnect failed: %s\n",
                    conn_ ? PQerrorMessage(conn_) : "null");
                if (conn_) { PQfinish(conn_); conn_ = nullptr; }
                return;
            }
            std::fprintf(stderr, "PGWriter: reconnected to PostgreSQL.\n");
        }
    }

    auto clearResult = [](PGresult *res) {
        if (res)
            PQclear(res);
    };

    auto execCommand = [this, &clearResult](const char *sql) -> bool {
        PGresult *res = PQexec(conn_, sql);
        const bool ok = res && PQresultStatus(res) == PGRES_COMMAND_OK;
        if (!ok)
            std::fprintf(stderr, "PGWriter command failed: %s\n", PQerrorMessage(conn_));
        clearResult(res);
        return ok;
    };

    auto execParamsText = [this, &clearResult](const char *sql, int nParams,
                                               const char *const *values) -> bool {
        PGresult *res = PQexecParams(conn_, sql, nParams, nullptr, values, nullptr, nullptr, 0);
        const bool ok = res && PQresultStatus(res) == PGRES_COMMAND_OK;
        if (!ok)
            std::fprintf(stderr, "PGWriter statement failed: %s\n", PQerrorMessage(conn_));
        clearResult(res);
        return ok;
    };

    auto rollback = [&execCommand]() {
        (void)execCommand("ROLLBACK");
    };

    if (!execCommand("BEGIN"))
        return;

    for (const DBEvent &event : batch)
    {
        switch (event.type)
        {
        case DBEventType::ORDER_ACCEPTED: {
            const std::string orderId = std::to_string(event.order_id);
            const std::string userId = std::to_string(event.user_id);
            const std::string symbolId = std::to_string(event.symbol_id);
            const std::string price = std::to_string(event.price);
            const std::string qty = std::to_string(event.qty);
            const std::string side = std::to_string(static_cast<int>(event.side));
            const std::string orderType = std::to_string(static_cast<int>(event.order_type));
            const std::string ts = std::to_string(event.timestamp);
            const char *values[] = {
                orderId.c_str(), userId.c_str(), symbolId.c_str(), price.c_str(),
                qty.c_str(), side.c_str(), orderType.c_str(), ts.c_str()};
            if (!execParamsText(kSqlOrdersUpsert, 8, values))
            {
                rollback();
                return;
            }
            break;
        }
        case DBEventType::ORDER_FILLED: {
            const std::string orderId = std::to_string(event.order_id);
            const std::string filledQty = std::to_string(event.qty - event.remaining);
            const std::string status = std::to_string(static_cast<int>(event.state));
            const std::string fillPriceStr = std::to_string(event.fill_price);
            const char *updateValues[] = {
                orderId.c_str(), filledQty.c_str(), status.c_str(), fillPriceStr.c_str()};
            if (!execParamsText(kSqlOrdersUpdate, 4, updateValues))
            {
                rollback();
                return;
            }

            if (event.side == Side::BUY)
            {
                const std::string symbolId = std::to_string(event.symbol_id);
                const std::string buyerOrderId = std::to_string(event.order_id);
                const std::string sellerOrderId = std::to_string(event.peer_order_id);
                const std::string buyerUserId = std::to_string(event.user_id);
                const std::string sellerUserId = std::to_string(event.peer_user_id);
                const std::string fillPrice = std::to_string(event.fill_price);
                const std::string fillQty = std::to_string(event.fill_qty);
                const std::string ts = std::to_string(event.timestamp);
                const char *tradeValues[] = {
                    symbolId.c_str(),      buyerOrderId.c_str(), sellerOrderId.c_str(),
                    buyerUserId.c_str(),   sellerUserId.c_str(), fillPrice.c_str(),
                    fillQty.c_str(),       ts.c_str()};
                if (!execParamsText(kSqlTradesInsert, 8, tradeValues))
                {
                    rollback();
                    return;
                }

                // Update buyer balances: deduct blocked funds
                {
                    const std::string buyerUserId = std::to_string(event.user_id);
                    const std::string notional = std::to_string(
                        event.fill_price * static_cast<int64_t>(event.fill_qty));
                    const char *bv[] = {buyerUserId.c_str(), notional.c_str()};
                    // Decrease blocked, leave available unchanged (already deducted at order time)
                    if (!execParamsText(
                        "UPDATE balances SET blocked = blocked - $2 WHERE user_id = $1",
                        2, bv))
                    {
                        rollback(); return;
                    }
                }
                // Update buyer holdings: add available_qty
                {
                    const std::string buyerUserId = std::to_string(event.user_id);
                    const std::string symId = std::to_string(event.symbol_id);
                    const std::string fillQtyStr = std::to_string(event.fill_qty);
                    const char *hv[] = {buyerUserId.c_str(), symId.c_str(), fillQtyStr.c_str()};
                    if (!execParamsText(
                        "INSERT INTO holdings (user_id, symbol_id, available_qty, blocked_qty) "
                        "VALUES ($1, $2, $3, 0) "
                        "ON CONFLICT (user_id, symbol_id) DO UPDATE "
                        "SET available_qty = holdings.available_qty + $3",
                        3, hv))
                    {
                        rollback(); return;
                    }
                }
            }

            // Always update the order-side participant's DB records.
            // For SELL side: deduct blocked holdings and add available balance.
            if (event.side == Side::SELL)
            {
                const std::string sellUserId = std::to_string(event.user_id);
                const std::string symId = std::to_string(event.symbol_id);
                const std::string fillQtyStr = std::to_string(event.fill_qty);
                const std::string notional = std::to_string(
                    event.fill_price * static_cast<int64_t>(event.fill_qty));
                // Deduct blocked holdings
                const char *hv[] = {sellUserId.c_str(), symId.c_str(), fillQtyStr.c_str()};
                if (!execParamsText(
                    "UPDATE holdings SET blocked_qty = blocked_qty - $3 "
                    "WHERE user_id = $1 AND symbol_id = $2",
                    3, hv))
                {
                    rollback(); return;
                }
                // Credit available balance
                const char *bv[] = {sellUserId.c_str(), notional.c_str()};
                if (!execParamsText(
                    "UPDATE balances SET available = available + $2 WHERE user_id = $1",
                    2, bv))
                {
                    rollback(); return;
                }
            }
            break;
        }
        case DBEventType::ORDER_CANCELLED: {
            const std::string orderId = std::to_string(event.order_id);
            const std::string filledQty = std::to_string(event.qty - event.remaining);
            const std::string status = "2";
            const std::string zeroPrice = "0";
            const char *values[] = {
                orderId.c_str(), filledQty.c_str(), status.c_str(), zeroPrice.c_str()};
            if (!execParamsText(kSqlOrdersUpdate, 4, values))
            {
                rollback();
                return;
            }
            break;
        }
        case DBEventType::ORDER_MODIFIED: {
            const std::string orderId = std::to_string(event.order_id);
            const std::string filledQty = std::to_string(event.qty - event.remaining);
            const std::string status = std::to_string(static_cast<int>(event.state));
            const std::string zeroPrice = "0";
            const char *values[] = {
                orderId.c_str(), filledQty.c_str(), status.c_str(), zeroPrice.c_str()};
            if (!execParamsText(kSqlOrdersUpdate, 4, values))
            {
                rollback();
                return;
            }
            break;
        }
        }
    }

    if (!execCommand("COMMIT"))
        rollback();
}
