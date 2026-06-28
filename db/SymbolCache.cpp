#include "SymbolCache.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include <libpq-fe.h>

void SymbolCache::loadFromDB(const std::string &pgConnString)
{
    symbolsById_.clear();
    idsByTicker_.clear();

    PGconn *conn = PQconnectdb(pgConnString.c_str());
    if (!conn || PQstatus(conn) != CONNECTION_OK)
    {
        std::fprintf(stderr, "SymbolCache::loadFromDB connection failed: %s\n",
                     conn ? PQerrorMessage(conn) : "null connection");
        if (conn)
            PQfinish(conn);
        return;
    }

    PGresult *res = PQexec(
        conn,
        "SELECT symbol_id, ticker, tick_size, lot_size, is_active FROM symbols ORDER BY symbol_id");
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::fprintf(stderr, "SymbolCache::loadFromDB query failed: %s\n", PQerrorMessage(conn));
        if (res)
            PQclear(res);
        PQfinish(conn);
        return;
    }

    const int rows = PQntuples(res);
    uint32_t maxId = 0;
    for (int row = 0; row < rows; ++row)
    {
        maxId = std::max(
            maxId,
            static_cast<uint32_t>(std::strtoul(PQgetvalue(res, row, 0), nullptr, 10)));
    }

    symbolsById_.resize(static_cast<std::size_t>(maxId) + 1u);
    for (int row = 0; row < rows; ++row)
    {
        const uint32_t symbolId = static_cast<uint32_t>(
            std::strtoul(PQgetvalue(res, row, 0), nullptr, 10));
        SymbolInfo info{
            symbolId,
            PQgetvalue(res, row, 1),
            std::strtoll(PQgetvalue(res, row, 2), nullptr, 10),
            static_cast<uint32_t>(std::strtoul(PQgetvalue(res, row, 3), nullptr, 10)),
            PQgetvalue(res, row, 4)[0] == 't'};
        symbolsById_[symbolId] = info;
        idsByTicker_[info.ticker] = symbolId;
    }

    PQclear(res);
    PQfinish(conn);
}

void SymbolCache::loadFromList(std::vector<SymbolInfo> symbols)
{
    symbolsById_.clear();
    idsByTicker_.clear();

    uint32_t maxId = 0;
    for (const auto &symbol : symbols)
        maxId = std::max(maxId, symbol.symbol_id);

    symbolsById_.resize(static_cast<std::size_t>(maxId) + 1u);
    for (auto &symbol : symbols)
    {
        idsByTicker_[symbol.ticker] = symbol.symbol_id;
        symbolsById_[symbol.symbol_id] = std::move(symbol);
    }
}

std::optional<uint32_t> SymbolCache::findByTicker(const std::string &ticker) const
{
    const auto it = idsByTicker_.find(ticker);
    if (it == idsByTicker_.end())
        return std::nullopt;
    return it->second;
}

std::optional<SymbolInfo> SymbolCache::findById(uint32_t id) const
{
    if (id >= symbolsById_.size())
        return std::nullopt;
    const SymbolInfo &info = symbolsById_[id];
    if (info.ticker.empty())
        return std::nullopt;
    return info;
}

uint32_t SymbolCache::count() const
{
    return static_cast<uint32_t>(symbolsById_.size());
}

bool SymbolCache::isValidTick(uint32_t symbolId, int64_t price) const
{
    const auto info = findById(symbolId);
    return info.has_value() && info->tick_size > 0 && (price % info->tick_size) == 0;
}

bool SymbolCache::isValidLot(uint32_t symbolId, uint32_t qty) const
{
    const auto info = findById(symbolId);
    return info.has_value() && info->lot_size > 0 && (qty % info->lot_size) == 0;
}
