#include "MarketController.h"

#include "api_server/db/PGPool.h"

namespace
{
Json::Value symbolRowToJson(const drogon::orm::Row &row)
{
    Json::Value item(Json::objectValue);
    item["symbol_id"] = row["symbol_id"].as<uint32_t>();
    item["ticker"] = row["ticker"].as<std::string>();
    item["company_name"] = row["company_name"].as<std::string>();
    item["tick_size"] = Json::Int64(row["tick_size"].as<int64_t>());
    item["lot_size"] = row["lot_size"].as<uint32_t>();
    return item;
}
} // namespace

void MarketController::listSymbols(const drogon::HttpRequestPtr &,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    try
    {
        const auto result = PGPool::client()->execSqlSync(
            "SELECT symbol_id, ticker, company_name, tick_size, lot_size FROM symbols "
            "WHERE is_active=true ORDER BY ticker");
        Json::Value out(Json::arrayValue);
        for (const auto &row : result)
            out.append(symbolRowToJson(row));
        callback(drogon::HttpResponse::newHttpJsonResponse(out));
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void MarketController::getSymbol(const drogon::HttpRequestPtr &,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 std::string ticker)
{
    try
    {
        const auto result = PGPool::client()->execSqlSync(
            "SELECT symbol_id, ticker, company_name, tick_size, lot_size FROM symbols "
            "WHERE is_active=true AND ticker=$1",
            ticker);
        if (result.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }
        callback(drogon::HttpResponse::newHttpJsonResponse(symbolRowToJson(result[0])));
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void MarketController::getTrades(const drogon::HttpRequestPtr &req,
                                 std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                 std::string ticker)
{
    try
    {
        const uint32_t limit = req->getOptionalParameter<uint32_t>("limit").value_or(100);
        const auto db = PGPool::client();
        const auto symbols = db->execSqlSync(
            "SELECT symbol_id FROM symbols WHERE is_active=true AND ticker=$1", ticker);
        if (symbols.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        // TradingView charting is a frontend concern; this trade history endpoint is the only backend input it needs.
        const auto result = db->execSqlSync(
            "SELECT price, qty, timestamp FROM trades WHERE symbol_id=$1 ORDER BY timestamp DESC LIMIT $2",
            symbols[0]["symbol_id"].as<uint32_t>(),
            limit);
        Json::Value out(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item(Json::objectValue);
            item["price"] = Json::Int64(row["price"].as<int64_t>());
            item["qty"] = row["qty"].as<int>();
            item["timestamp"] = Json::UInt64(row["timestamp"].as<uint64_t>());
            out.append(item);
        }
        callback(drogon::HttpResponse::newHttpJsonResponse(out));
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}
