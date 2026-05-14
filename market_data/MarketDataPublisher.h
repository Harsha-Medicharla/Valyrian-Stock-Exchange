#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#include "CandleBuilder.h"
#include "shared/queues/EventSPSC.h"
#include "shared/types/Events.h"

class MarketDataPublisher
{
private:
    std::vector<EventSPSC<TradeEvent>> *tradeQueues_{nullptr};
    uint32_t numSymbols_{0};
    uint16_t wsPort_{0};
    CandleBuilder candleBuilder_;
    std::vector<Price> lastPrice_;
    std::vector<Price> lastBestBid_;
    std::vector<Price> lastBestAsk_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    void run();
    void processTradeEvent(uint32_t sym, const TradeEvent &ev);

public:
    MarketDataPublisher(std::vector<EventSPSC<TradeEvent>> &tradeQueues, uint32_t numSymbols,
                        uint16_t wsPort)
        : tradeQueues_(&tradeQueues),
          numSymbols_(numSymbols),
          wsPort_(wsPort),
          candleBuilder_(numSymbols),
          lastPrice_(numSymbols, 0),
          lastBestBid_(numSymbols, 0),
          lastBestAsk_(numSymbols, 0)
    {
        (void)wsPort_;
    }

    void start();
    void stop() noexcept;
    void join();
};
