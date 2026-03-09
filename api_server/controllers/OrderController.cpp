#include "OrderController.h"

#include <cstdlib>
#include <json/json.h>

#include "api_server/db/PGPool.h"
#include "api_server/db/RedisPool.h"
#include "api_server/middleware/SessionValidator.h"

void OrderController::listOrders(const drogon::HttpRequestPtr &req,
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
    const uint32_t limit =
        req->getOptionalParameter<uint32_t>("limit").value_or(200);

    try
    {
        const auto db = PGPool::client();
        const auto symbolId = req->getOptionalParameter<uint32_t>("symbol_id");
        const std::string status = req->getParameter("status");
        Json::Value out(Json::arrayValue);

        auto appendRows = [&out](const auto &result) {
            for (const auto &row : result)
            {
                Json::Value item(Json::objectValue);
                item["order_id"] = Json::UInt64(row["order_id"].template as<uint64_t>());
                item["symbol_id"] = row["symbol_id"].template as<uint32_t>();
                item["price"] = Json::Int64(row["price"].template as<int64_t>());
                item["qty"] = row["qty"].template as<int>();
                item["filled_qty"] = row["filled_qty"].template as<int>();
                item["side"] = row["side"].template as<int>();
                item["type"] = row["type"].template as<int>();
                item["status"] = row["status"].template as<int>();
                item["timestamp"] = Json::UInt64(row["timestamp"].template as<uint64_t>());
                out.append(item);
            }
        };

        if (!symbolId && status.empty())
        {
            const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                "FROM orders WHERE user_id=$1 ORDER BY timestamp DESC LIMIT $2",
                                                *userId, limit);
            appendRows(result);
        }
        else if (symbolId && status.empty())
        {
            const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                "FROM orders WHERE user_id=$1 AND symbol_id=$2 ORDER BY timestamp DESC LIMIT $3",
                                                *userId, *symbolId, limit);
            appendRows(result);
        }
        else if (status == "pending")
        {
            if (symbolId)
            {
                const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                    "FROM orders WHERE user_id=$1 AND symbol_id=$2 AND status IN (0,1) ORDER BY timestamp DESC LIMIT $3",
                                                    *userId, *symbolId, limit);
                appendRows(result);
            }
            else
            {
                const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                    "FROM orders WHERE user_id=$1 AND status IN (0,1) ORDER BY timestamp DESC LIMIT $2",
                                                    *userId, limit);
                appendRows(result);
            }
        }
        else
        {
            const int statusValue = std::atoi(status.c_str());
            if (symbolId)
            {
                const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                    "FROM orders WHERE user_id=$1 AND symbol_id=$2 AND status=$3 ORDER BY timestamp DESC LIMIT $4",
                                                    *userId, *symbolId, statusValue, limit);
                appendRows(result);
            }
            else
            {
                const auto result = db->execSqlSync("SELECT order_id, symbol_id, price, qty, filled_qty, side, type, status, timestamp "
                                                    "FROM orders WHERE user_id=$1 AND status=$2 ORDER BY timestamp DESC LIMIT $3",
                                                    *userId, statusValue, limit);
                appendRows(result);
            }
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

void OrderController::cancelOrder(const drogon::HttpRequestPtr &req,
                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                                  uint64_t orderId)
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
        const auto db = PGPool::client();
        const auto result = db->execSqlSync(
            "SELECT user_id, symbol_id FROM orders WHERE order_id=$1", orderId);
        if (result.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }
        if (result[0]["user_id"].as<uint64_t>() != *userId)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k403Forbidden);
            callback(resp);
            return;
        }

        Json::Value cancel(Json::objectValue);
        cancel["type"] = "Cancel";
        cancel["order_id"] = Json::UInt64(orderId);
        cancel["user_id"] = Json::UInt64(*userId);
        cancel["symbol_id"] = result[0]["symbol_id"].as<uint32_t>();
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        if (!RedisPool::instance().publish("vse:orders:cancel", Json::writeString(writer, cancel)))
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k503ServiceUnavailable);
            callback(resp);
            return;
        }

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k202Accepted);
        callback(resp);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}
