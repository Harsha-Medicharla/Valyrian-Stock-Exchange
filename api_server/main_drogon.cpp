#include <cstdlib>
#include <string>

#include <drogon/drogon.h>

#include "db/PGPool.h"
#include "db/RedisPool.h"
#include "trade_server/ThreadAffinity.h"

namespace
{
[[nodiscard]] int envInt(const char *key, int fallback) noexcept
{
    if (const char *value = std::getenv(key))
    {
        const int parsed = std::atoi(value);
        if (parsed > 0)
            return parsed;
    }
    return fallback;
}

[[nodiscard]] std::string envString(const char *key, const char *fallback)
{
    if (const char *value = std::getenv(key); value && value[0] != '\0')
        return value;
    return fallback;
}
} // namespace

int main()
{
    if (const auto core = vse::threads::coreForRole("OS + Drogon"))
        vse::threads::pinToCore(*core);

    PGPool::init(envString("VSE_PG_CONN", ""));
    RedisPool::instance().init(envString("REDIS_HOST", "127.0.0.1"), envInt("REDIS_PORT", 6379));

    drogon::app().addListener("0.0.0.0", envInt("API_PORT", 8080));
    drogon::app().setThreadNum(envInt("API_THREADS", 4));
    drogon::app().setLogLevel(trantor::Logger::kInfo);
    drogon::app().registerHandler(
        "/healthz",
        [](const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            Json::Value body(Json::objectValue);
            body["status"] = "ok";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        },
        {drogon::Get});
    drogon::app().run();
    return 0;
}
