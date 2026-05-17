#include "MarketDataPublisher.h"

#include <App.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "trade_server/ThreadAffinity.h"
#include "utils/SpinWait.h"

extern "C" void us_listen_socket_close(int ssl, us_listen_socket_t *ls);

namespace
{
struct WsClientData
{
};

[[nodiscard]] std::string topicForSymbol(uint32_t symbolId)
{
    return "sym_" + std::to_string(symbolId);
}

[[nodiscard]] std::string candleTopicForSymbol(uint32_t symbolId)
{
    return topicForSymbol(symbolId) + "_candles";
}

[[nodiscard]] bool parseAction(std::string_view message, std::string_view &action)
{
    const std::size_t key = message.find("\"action\"");
    if (key == std::string_view::npos)
        return false;
    const std::size_t colon = message.find(':', key);
    const std::size_t firstQuote = message.find('"', colon + 1);
    const std::size_t secondQuote = message.find('"', firstQuote + 1);
    if (colon == std::string_view::npos || firstQuote == std::string_view::npos ||
        secondQuote == std::string_view::npos)
        return false;
    action = message.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    return true;
}

[[nodiscard]] bool parseSymbolId(std::string_view message, uint32_t &symbolId)
{
    const std::size_t key = message.find("\"symbol_id\"");
    if (key == std::string_view::npos)
        return false;
    const std::size_t colon = message.find(':', key);
    if (colon == std::string_view::npos)
        return false;
    const std::size_t start = message.find_first_of("0123456789", colon + 1);
    if (start == std::string_view::npos)
        return false;
    const std::size_t end = message.find_first_not_of("0123456789", start);
    symbolId = static_cast<uint32_t>(
        std::strtoul(std::string(message.substr(start, end - start)).c_str(), nullptr, 10));
    return true;
}
} // namespace

void MarketDataPublisher::publishToTopic(std::string topic, std::string payload)
{
    uWS::Loop *loop = loop_;
    uWS::App *app = app_;
    if (!loop || !app)
        return;

    loop->defer([this, topic = std::move(topic), payload = std::move(payload)]() mutable {
        if (!app_)
            return;
        app_->publish(topic, payload, uWS::OpCode::TEXT, false);
    });
}

void MarketDataPublisher::processTradeEvent(uint32_t sym, const TradeEvent &ev)
{
    lastPrice_[sym].store(ev.price, std::memory_order_relaxed);
    lastBestBid_[sym].store(ev.best_bid, std::memory_order_relaxed);
    lastBestAsk_[sym].store(ev.best_ask, std::memory_order_relaxed);

    std::string tradeJson = "{\"type\":\"trade\",\"symbol_id\":" + std::to_string(sym) +
                            ",\"price\":" + std::to_string(ev.price) +
                            ",\"qty\":" + std::to_string(ev.qty) +
                            ",\"best_bid\":" + std::to_string(ev.best_bid) +
                            ",\"best_ask\":" + std::to_string(ev.best_ask) +
                            ",\"ts\":" + std::to_string(ev.timestamp) + "}";
    publishToTopic(topicForSymbol(sym), std::move(tradeJson));

    std::string candleResult = candleBuilder_.onTrade(sym, ev.price, ev.qty, ev.timestamp);
    if (!candleResult.empty())
    {
        std::size_t pos = 0;
        while (pos < candleResult.size())
        {
            const std::size_t nl = candleResult.find('\n', pos);
            const std::size_t end =
                (nl == std::string::npos) ? candleResult.size() : nl;
            publishToTopic(candleTopicForSymbol(sym),
                           candleResult.substr(pos, end - pos));
            pos = (nl == std::string::npos) ? candleResult.size() : nl + 1;
        }
    }
}

void MarketDataPublisher::run()
{
    if (const auto core = vse::threads::coreForRole("FeedPublisher"))
        vse::threads::pinToCore(*core);

    while (running_.load(std::memory_order_relaxed))
    {
        bool anyWork = false;
        for (uint32_t sym = 0; sym < numSymbols_; ++sym)
        {
            EventSPSC<TradeEvent> &q = (*tradeQueues_)[sym];
            while (TradeEvent *ev = q.front())
            {
                processTradeEvent(sym, *ev);
                q.pop();
                anyWork = true;
            }
        }
        if (!anyWork)
            ems::pause();
    }
}

void MarketDataPublisher::runWs()
{
    uWS::App app;
    app_ = &app;
    loop_ = uWS::Loop::get();
    loopPromise_.set_value(loop_);

    app.ws<WsClientData>(
           "/*",
           {
               .compression = uWS::DISABLED,
               .maxPayloadLength = 16 * 1024,
               .idleTimeout = 120,
               .open = [](auto * /*ws*/) {},
               .message =
                   [this](auto *ws, std::string_view message, uWS::OpCode op) {
                       if (op != uWS::OpCode::TEXT)
                           return;

                       std::string_view action;
                       uint32_t symbolId = 0;
                       if (!parseAction(message, action) || !parseSymbolId(message, symbolId) ||
                           symbolId >= numSymbols_)
                           return;

                       const std::string topic = topicForSymbol(symbolId);
                       if (action == "subscribe")
                       {
                           ws->subscribe(topic);
                           const std::string snapshot =
                               "{\"type\":\"snapshot\",\"symbol_id\":" + std::to_string(symbolId) +
                               ",\"last_price\":" +
                               std::to_string(lastPrice_[symbolId].load(std::memory_order_relaxed)) +
                               ",\"best_bid\":" +
                               std::to_string(lastBestBid_[symbolId].load(std::memory_order_relaxed)) +
                               ",\"best_ask\":" +
                               std::to_string(lastBestAsk_[symbolId].load(std::memory_order_relaxed)) + "}";
                           ws->send(snapshot, uWS::OpCode::TEXT);
                       }
                       else if (action == "unsubscribe")
                       {
                           ws->unsubscribe(topic);
                       }
                   }})
        .listen(wsPort_, [this](auto *token) {
            listenSocket_ = token;
            if (!token)
                std::fprintf(stderr, "vse_market_data: failed to bind WebSocket port %u\n",
                             static_cast<unsigned>(wsPort_));
        })
        .run();

    listenSocket_ = nullptr;
    loop_ = nullptr;
    app_ = nullptr;
}

void MarketDataPublisher::start()
{
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&MarketDataPublisher::run, this);
    wsThread_ = std::thread(&MarketDataPublisher::runWs, this);
    loop_ = loopFuture_.get();
}

void MarketDataPublisher::stop() noexcept
{
    running_.store(false, std::memory_order_release);
    if (loop_)
    {
        loop_->defer([this]() {
            if (listenSocket_)
                us_listen_socket_close(0, listenSocket_);
        });
    }
}

void MarketDataPublisher::join()
{
    if (thread_.joinable())
        thread_.join();
    if (wsThread_.joinable())
        wsThread_.join();
}
