#include <cstdlib>
#include <fstream>
#include <json/json.h>
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

[[nodiscard]] Json::Value loadConfig()
{
    std::ifstream in("config.json");
    if (!in)
        return {};
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;
    if (!Json::parseFromStream(reader, in, &root, &errs))
        return {};
    if (root.isMember("api_server") && root["api_server"].isObject())
        return root["api_server"];
    return root;
}

[[nodiscard]] int configInt(const Json::Value &cfg, const char *jsonKey,
                            const char *envKey, int fallback) noexcept
{
    if (cfg.isMember(jsonKey) && cfg[jsonKey].isInt())
        return cfg[jsonKey].asInt();
    return envInt(envKey, fallback);
}

[[nodiscard]] std::string envString(const char *key, const char *fallback)
{
    if (const char *value = std::getenv(key); value && value[0] != '\0')
        return value;
    return fallback;
}

[[nodiscard]] std::string configString(const Json::Value &cfg, const char *jsonKey,
                                       const char *envKey, const char *fallback)
{
    if (cfg.isMember(jsonKey) && cfg[jsonKey].isString())
        return cfg[jsonKey].asString();
    return envString(envKey, fallback);
}

void applyCoreMapConfig(const Json::Value &cfg)
{
    if (!cfg.isMember("core_pinning") || !cfg["core_pinning"].isObject())
        return;
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    const std::string coreMap = Json::writeString(writer, cfg["core_pinning"]);
    setenv("VSE_CORE_MAP_JSON", coreMap.c_str(), 1);
}
} // namespace

int main()
{
    const Json::Value cfg = loadConfig();
    applyCoreMapConfig(cfg);

    if (const auto core = vse::threads::coreForRole("OS + Drogon"))
        vse::threads::pinToCore(*core);

    PGPool::init(configString(cfg, "pg_conn", "VSE_PG_CONN", ""));
    RedisPool::instance().init(configString(cfg, "redis_host", "REDIS_HOST", "127.0.0.1"),
                               configInt(cfg, "redis_port", "REDIS_PORT", 6379));

    drogon::app().addListener("0.0.0.0", configInt(cfg, "api_port", "API_PORT", 8080));
    drogon::app().setThreadNum(configInt(cfg, "api_threads", "API_THREADS", 4));
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
