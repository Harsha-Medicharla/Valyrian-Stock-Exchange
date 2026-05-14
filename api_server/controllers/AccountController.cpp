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
