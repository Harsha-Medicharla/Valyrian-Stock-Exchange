#include <csignal>
#include <cstdio>
#include <cstdlib>

#include "config/TradeServerConfig.h"
#include "core/EMSCore.h"
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

    EMSCore ems(TradeServerConfig::IO_THREADS, /*numSymbols=*/16);
    WebSocketServer ws(ems, TradeServerConfig::WS_PORT);
    RespThread resp(ems, ws);

    ems.start();
    resp.start();

    std::fprintf(stderr, "vse_trade_server: WebSocket listening on port %u (Ctrl+C to exit)\n",
                 static_cast<unsigned>(TradeServerConfig::WS_PORT));
    if (std::getenv("VSE_DEV_SKIP_AUTH"))
        std::fprintf(stderr, "vse_trade_server: VSE_DEV_SKIP_AUTH is set; Bearer validation is bypassed.\n");
    std::fflush(stderr);

    ws.run();

    resp.stop();
    resp.join();
    ems.stop();
    ems.join();
    return 0;
}
