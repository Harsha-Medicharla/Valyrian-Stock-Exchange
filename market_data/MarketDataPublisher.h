#pragma once
#include <App.h>

#include <atomic>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#include "CandleBuilder.h"
#include "shared/queues/EventSPSC.h"
#include "shared/types/Events.h"

struct us_listen_socket_t;
namespace uWS
{
class Loop;
template <bool, bool, typename>
class WebSocket;
}

class MarketDataPublisher
{
private:
    std::vector<EventSPSC<TradeEvent>> *tradeQueues_{nullptr};
    std::vector<EventSPSC<BookUpdateEvent>> *bookUpdateQueues_{nullptr};
    uint32_t numSymbols_{0};
    uint16_t wsPort_{0};
    CandleBuilder candleBuilder_;
    std::vector<std::atomic<Price>> lastPrice_;
    std::vector<std::atomic<Price>> lastBestBid_;
    std::vector<std::atomic<Price>> lastBestAsk_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::thread wsThread_;
    uWS::App *app_{nullptr};
    uWS::Loop *loop_{nullptr};
    us_listen_socket_t *listenSocket_{nullptr};
    std::promise<uWS::Loop *> loopPromise_;
    std::future<uWS::Loop *> loopFuture_{loopPromise_.get_future()};

    void run();
    void processTradeEvent(uint32_t sym, const TradeEvent &ev);
    void processBookUpdateEvent(uint32_t sym, const BookUpdateEvent &ev);
    void runWs();
    void publishToTopic(std::string topic, std::string payload);

public:
    MarketDataPublisher(std::vector<EventSPSC<TradeEvent>> &tradeQueues,
                        std::vector<EventSPSC<BookUpdateEvent>> &bookUpdateQueues,
                        uint32_t numSymbols, uint16_t wsPort)
        : tradeQueues_(&tradeQueues),
          bookUpdateQueues_(&bookUpdateQueues),
          numSymbols_(numSymbols),
          wsPort_(wsPort),
          candleBuilder_(numSymbols),
          lastPrice_(numSymbols),
          lastBestBid_(numSymbols),
          lastBestAsk_(numSymbols)
    {
        for (uint32_t sym = 0; sym < numSymbols_; ++sym)
        {
            lastPrice_[sym].store(0, std::memory_order_relaxed);
            lastBestBid_[sym].store(0, std::memory_order_relaxed);
            lastBestAsk_[sym].store(0, std::memory_order_relaxed);
        }
    }

    void start();
    void stop() noexcept;
    void join();
};
