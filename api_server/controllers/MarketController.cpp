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
    try {
        const auto db = PGPool::client();
        // Resolve ticker string (e.g. "AAPL") to symbol_id integer
        const auto symRes = db->execSqlSync("SELECT symbol_id FROM symbols WHERE ticker=$1", ticker);
        if (symRes.empty()) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }
        const uint32_t symbolId = symRes[0]["symbol_id"].as<uint32_t>();

        // Query historical entries safely using the ID
        const auto result = db->execSqlSync(
            "SELECT price, qty, timestamp FROM trades WHERE symbol_id=$1 ORDER BY timestamp DESC", symbolId);

        Json::Value arr(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value trade;
            trade["price"] = row["price"].as<int64_t>();
            trade["qty"] = row["qty"].as<int32_t>();
            trade["timestamp"] = row["timestamp"].as<std::string>();
            arr.append(trade);
        }
        auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
        callback(resp);
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}
