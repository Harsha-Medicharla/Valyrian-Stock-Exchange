#include "SessionValidator.h"

#include <cstdlib>

#include <drogon/HttpResponse.h>

#include "api_server/db/RedisPool.h"

namespace
{
    [[nodiscard]] drogon::HttpResponsePtr unauthorizedResponse()
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        return resp;
    }
}

std::optional<std::string_view> SessionValidator::bearerToken(std::string_view header)
{
    static constexpr std::string_view prefix = "Bearer ";
    if (header.size() < prefix.size() || header.substr(0, prefix.size()) != prefix)
        return std::nullopt;
    return header.substr(prefix.size());
}

void SessionValidator::doFilter(const drogon::HttpRequestPtr &req,
                                drogon::FilterCallback &&fcb,
                                drogon::FilterChainCallback &&fccb)
{
    const auto token = bearerToken(req->getHeader("Authorization"));
    if (!token)
    {
        fcb(unauthorizedResponse());
        return;
    }

    const auto user = RedisPool::instance().get("session:" + std::string(*token));
    if (!user)
    {
        fcb(unauthorizedResponse());
        return;
    }

    req->attributes()->insert("user_id", static_cast<uint64_t>(std::strtoull(user->c_str(), nullptr, 10)));
    fccb();
}

std::optional<uint64_t> SessionValidator::userId(const drogon::HttpRequestPtr &req)
{
    try
    {
        return req->attributes()->get<uint64_t>("user_id");
    }
    catch (...)
    {
        return std::nullopt;
    }
}
