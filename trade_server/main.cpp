#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <json/json.h>
#include <string>
#include <thread>

#include "config/TradeServerConfig.h"
#include "core/EMSCore.h"
#include "db/BalanceCacheBootstrap.h"
#include "db/DBWriter.h"
#include "db/SymbolCache.h"
#include "market_data/MarketDataPublisher.h"
#include "network/WebSocketServer.h"
#include "resp/RespThread.h"

namespace
{
    std::atomic<bool> running{true};
    WebSocketServer *activeWs{nullptr};

    void onSignal(int /*sig*/)
    {
        running.store(false, std::memory_order_release);
        if (activeWs)
            activeWs->stop();
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
        if (root.isMember("trade_server") && root["trade_server"].isObject())
            return root["trade_server"];
        return root;
    }

    [[nodiscard]] int configInt(const Json::Value &cfg, const char *jsonKey,
                                const char *envKey, int fallback) noexcept
    {
        if (cfg.isMember(jsonKey) && cfg[jsonKey].isInt())
            return cfg[jsonKey].asInt();
        if (const char *value = std::getenv(envKey))
        {
            const int parsed = std::atoi(value);
            if (parsed > 0)
                return parsed;
        }
        return fallback;
    }

    [[nodiscard]] std::size_t configSize(const Json::Value &cfg, const char *jsonKey,
                                         const char *envKey, std::size_t fallback) noexcept
    {
        return static_cast<std::size_t>(configInt(cfg, jsonKey, envKey, static_cast<int>(fallback)));
    }

    [[nodiscard]] std::string configString(const Json::Value &cfg, const char *jsonKey,
                                           const char *envKey, const char *fallback)
    {
        if (cfg.isMember(jsonKey) && cfg[jsonKey].isString())
            return cfg[jsonKey].asString();
        if (const char *value = std::getenv(envKey); value && value[0] != '\0')
            return value;
        return fallback;
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
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    const Json::Value cfg = loadConfig();
    applyCoreMapConfig(cfg);

    const std::string pgConn = configString(cfg, "pg_conn", "VSE_PG_CONN", "");
    const std::uint16_t wsPort = static_cast<std::uint16_t>(
        configInt(cfg, "ws_port", "VSE_WS_PORT", TradeServerConfig::WS_PORT));
    const std::uint16_t mdpPort = static_cast<std::uint16_t>(
        configInt(cfg, "mdp_ws_port", "VSE_MDP_WS_PORT", TradeServerConfig::MDP_WS_PORT));
    const std::size_t ioThreads = configSize(cfg, "io_threads", "VSE_IO_THREADS",
                                             TradeServerConfig::IO_THREADS);
    const std::string redisHost = configString(cfg, "redis_host", "REDIS_HOST", "127.0.0.1");
    const int redisPort = configInt(cfg, "redis_port", "REDIS_PORT", 6379);
    setenv("REDIS_HOST", redisHost.c_str(), 1);
    setenv("REDIS_PORT", std::to_string(redisPort).c_str(), 1);
    setenv("VSE_IO_THREADS", std::to_string(ioThreads).c_str(), 1);

    SymbolCache symbolCache;
    symbolCache.loadFromDB(pgConn);

    EMSCore ems(ioThreads, symbolCache.count(), symbolCache);
    bootstrapBalanceCache(ems.balanceCache(), pgConn, static_cast<uint32_t>(ems.numSymbols()));

    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), static_cast<uint32_t>(ems.numSymbols()),
                      ems.balanceCache(), pgConn);
    MarketDataPublisher marketData(ems.tradeQueues(), ems.bookUpdateQueues(),
                                   static_cast<uint32_t>(ems.numSymbols()), mdpPort);
    WebSocketServer ws(ems, wsPort);
    RespThread resp(ems, ws);
    activeWs = &ws;

    ems.start();
    dbWriter.start();
    marketData.start();
    resp.start();
    ws.startCancelSubscriber();
    ws.startBalanceSyncSubscriber(ems.balanceCache());

    std::fprintf(stderr, "vse_trade_server: WebSocket listening on port %u (Ctrl+C to exit)\n",
                 static_cast<unsigned>(wsPort));
    std::fprintf(stderr, "vse_market_data: WebSocket listening on port %u (subscribe to symbol feeds)\n",
                 static_cast<unsigned>(mdpPort));
    if (std::getenv("VSE_DEV_SKIP_AUTH"))
        std::fprintf(stderr, "vse_trade_server: VSE_DEV_SKIP_AUTH is set; Bearer validation is bypassed.\n");
    std::fflush(stderr);

    ws.run();
    running.store(false, std::memory_order_release);

    ws.stopCancelSubscriber();
    ws.stopBalanceSyncSubscriber();
    resp.stop();
    resp.join();
    marketData.stop();
    marketData.join();
    dbWriter.stop();
    dbWriter.join();
    ems.stop();
    ems.join();
    activeWs = nullptr;
    return 0;
}
