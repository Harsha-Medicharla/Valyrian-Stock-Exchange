#include "MarketDataPublisher.h"

#include "utils/SpinWait.h"

void MarketDataPublisher::processTradeEvent(uint32_t sym, const TradeEvent &ev)
{
    lastPrice_[sym] = ev.price;
    lastBestBid_[sym] = ev.best_bid;
    lastBestAsk_[sym] = ev.best_ask;
    (void)candleBuilder_.onTrade(sym, ev.price, ev.qty, ev.timestamp);
}

void MarketDataPublisher::run()
{
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

void MarketDataPublisher::start()
{
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&MarketDataPublisher::run, this);
}

void MarketDataPublisher::stop() noexcept { running_.store(false, std::memory_order_release); }

void MarketDataPublisher::join()
{
    if (thread_.joinable())
        thread_.join();
}
