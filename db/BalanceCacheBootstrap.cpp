#include "BalanceCacheBootstrap.h"

#include <cstdlib>
#include <cstdio>

#include <libpq-fe.h>

#include "ems/pipeline/BalanceCache.h"

void bootstrapBalanceCache(
    BalanceCache &cache,
    const std::string &pgConnString,
    uint32_t numSymbols)
{
    (void)numSymbols;

    // DB balances are the durable baseline. Startup must replay the matching-engine WAL after this
    // bootstrap so in-flight orders and cancellations reconcile BalanceCache to the engine's truth.

    PGconn *conn = PQconnectdb(pgConnString.c_str());
    if (!conn || PQstatus(conn) != CONNECTION_OK)
    {
        std::fprintf(stderr, "bootstrapBalanceCache: PostgreSQL connection failed: %s\n",
                     conn ? PQerrorMessage(conn) : "null connection");
        if (conn)
            PQfinish(conn);
        return;
    }

    PGresult *balances = PQexec(conn, "SELECT user_id, available, blocked FROM balances");
    if (balances && PQresultStatus(balances) == PGRES_TUPLES_OK)
    {
        const int rows = PQntuples(balances);
        for (int row = 0; row < rows; ++row)
        {
            const uint32_t userId = static_cast<uint32_t>(
                std::strtoul(PQgetvalue(balances, row, 0), nullptr, 10));
            const int64_t available = std::strtoll(PQgetvalue(balances, row, 1), nullptr, 10);
            const int64_t blocked = std::strtoll(PQgetvalue(balances, row, 2), nullptr, 10);
            cache.setBalance(userId, available, blocked);
        }
    }
    else
    {
        std::fprintf(stderr, "bootstrapBalanceCache: balances query failed: %s\n", PQerrorMessage(conn));
    }
    if (balances)
        PQclear(balances);

    PGresult *holdings =
        PQexec(conn, "SELECT user_id, symbol_id, available_qty, blocked_qty FROM holdings");
    if (holdings && PQresultStatus(holdings) == PGRES_TUPLES_OK)
    {
        const int rows = PQntuples(holdings);
        for (int row = 0; row < rows; ++row)
        {
            const uint32_t userId = static_cast<uint32_t>(
                std::strtoul(PQgetvalue(holdings, row, 0), nullptr, 10));
            const uint32_t symbolId = static_cast<uint32_t>(
                std::strtoul(PQgetvalue(holdings, row, 1), nullptr, 10));
            const int32_t availableQty = static_cast<int32_t>(
                std::strtol(PQgetvalue(holdings, row, 2), nullptr, 10));
            const int32_t blockedQty = static_cast<int32_t>(
                std::strtol(PQgetvalue(holdings, row, 3), nullptr, 10));
            cache.setHoldings(userId, symbolId, availableQty, blockedQty);
        }
    }
    else
    {
        std::fprintf(stderr, "bootstrapBalanceCache: holdings query failed: %s\n", PQerrorMessage(conn));
    }
    if (holdings)
        PQclear(holdings);

    PQfinish(conn);
}
