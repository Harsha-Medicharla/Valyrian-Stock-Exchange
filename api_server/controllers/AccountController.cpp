#include "AccountController.h"

#include "api_server/db/PGPool.h"
#include "api_server/middleware/SessionValidator.h"

void AccountController::getAccount(const drogon::HttpRequestPtr &req,
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
            "SELECT name, email FROM users WHERE user_id=$1", *userId);
        if (result.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
            return;
        }

        Json::Value out(Json::objectValue);
        out["name"] = result[0]["name"].as<std::string>();
        out["email"] = result[0]["email"].as<std::string>();
        callback(drogon::HttpResponse::newHttpJsonResponse(out));
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void AccountController::updateAccount(const drogon::HttpRequestPtr &req,
                                      std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto userId = SessionValidator::userId(req);
    const auto body = req->getJsonObject();
    if (!userId || !body || !body->isMember("name"))
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try
    {
        PGPool::client()->execSqlSync("UPDATE users SET name=$1 WHERE user_id=$2",
                                      (*body)["name"].asString(),
                                      *userId);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void AccountController::deposit(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto userId = SessionValidator::userId(req);
    const auto body = req->getJsonObject();
    if (!userId || !body || !body->isMember("amount"))
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const int64_t amount = (*body)["amount"].asInt64();
    if (amount <= 0)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try
    {
        const auto db = PGPool::client();
        // 1. Persist directly to PostgreSQL database
        db->execSqlSync(
            "INSERT INTO balances (user_id, available, blocked) VALUES ($1, $2, 0) "
            "ON CONFLICT (user_id) DO UPDATE SET available = balances.available + $2",
            *userId, amount);

        // 2. Publish balance synchronization payload to Redis to inform the engine layers
        Json::Value balanceSync(Json::objectValue);
        balanceSync["type"] = "Deposit";
        balanceSync["user_id"] = Json::UInt64(*userId);
        balanceSync["amount"] = Json::Int64(amount);
        
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        RedisPool::instance().publish("vse:balances:sync", Json::writeString(writer, balanceSync));

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}
