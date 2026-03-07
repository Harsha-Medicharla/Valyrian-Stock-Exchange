#include "BalanceController.h"

#include "api_server/db/PGPool.h"
#include "api_server/middleware/SessionValidator.h"

void BalanceController::getBalance(const drogon::HttpRequestPtr &req,
                                   std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto userId = SessionValidator::userId(req);
    if (!userId)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    try
    {
        const auto result = PGPool::client()->execSqlSync(
            "SELECT available, blocked FROM balances WHERE user_id=$1", *userId);
        Json::Value out(Json::objectValue);
        out["available"] = result.empty() ? Json::Int64(0) : Json::Int64(result[0]["available"].as<int64_t>());
        out["blocked"] = result.empty() ? Json::Int64(0) : Json::Int64(result[0]["blocked"].as<int64_t>());
        callback(drogon::HttpResponse::newHttpJsonResponse(out));
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void BalanceController::getHoldings(const drogon::HttpRequestPtr &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto userId = SessionValidator::userId(req);
    if (!userId)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }

    try
    {
        const auto result = PGPool::client()->execSqlSync(
            "SELECT symbol_id, available_qty, blocked_qty FROM holdings WHERE user_id=$1 ORDER BY symbol_id",
            *userId);
        Json::Value out(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item(Json::objectValue);
            item["symbol_id"] = row["symbol_id"].as<uint32_t>();
            item["available_qty"] = row["available_qty"].as<int>();
            item["blocked_qty"] = row["blocked_qty"].as<int>();
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
