#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>

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
    void onSignal(int /*sig*/) { std::quick_exit(0); }
} // namespace

int main()
{
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    const char *pgConnString = std::getenv("VSE_PG_CONN");
    const std::string pgConn = pgConnString ? pgConnString : "";

    SymbolCache symbolCache;
    symbolCache.loadFromDB(pgConn);

    EMSCore ems(TradeServerConfig::IO_THREADS, symbolCache.count(), symbolCache);
    bootstrapBalanceCache(ems.balanceCache(), pgConn, static_cast<uint32_t>(ems.numSymbols()));

    DBWriter dbWriter(ems.ingressDbQueues(), ems.engineDbQueues(), static_cast<uint32_t>(ems.numSymbols()),
                      ems.balanceCache(), pgConn);
    MarketDataPublisher marketData(ems.tradeQueues(), static_cast<uint32_t>(ems.numSymbols()),
                                   TradeServerConfig::MDP_WS_PORT);
    WebSocketServer ws(ems, TradeServerConfig::WS_PORT);
    RespThread resp(ems, ws);

    ems.start();
    dbWriter.start();
    marketData.start();
    resp.start();

    std::fprintf(stderr, "vse_trade_server: WebSocket listening on port %u (Ctrl+C to exit)\n",
                 static_cast<unsigned>(TradeServerConfig::WS_PORT));
    std::fprintf(stderr, "vse_market_data: WebSocket listening on port %u (subscribe to symbol feeds)\n",
                 static_cast<unsigned>(TradeServerConfig::MDP_WS_PORT));
    if (std::getenv("VSE_DEV_SKIP_AUTH"))
        std::fprintf(stderr, "vse_trade_server: VSE_DEV_SKIP_AUTH is set; Bearer validation is bypassed.\n");
    std::fflush(stderr);

    ws.run();

    resp.stop();
    resp.join();
    marketData.stop();
    marketData.join();
    dbWriter.stop();
    dbWriter.join();
    ems.stop();
    ems.join();
    return 0;
}
