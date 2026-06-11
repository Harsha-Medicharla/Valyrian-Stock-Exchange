#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "shared/types/Events.h"
#include "shared/queues/EventSPSC.h"
#include "../config/EMSConfig.h"
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "../queues/RejectionBitset.h"
#include "IngressWorker.h"
#include "Dispatcher.h"
#include "../routing/SymbolRouter.h"
#include "../pipeline/MarketState.h"
#include "../pipeline/RateLimiter.h"
#include "../pipeline/BalanceCache.h"
#include "../../db/SymbolCache.h"
#include "../../MatchingEngine/include/MatchingEngine.h"

class EMSCore
{
private:
    size_t numWorkers_;
    size_t numSymbols_;
    std::vector<SPSCQueueWrapper> spscQueues_;
    RateLimiter rateLimiter_;
    BalanceCache balanceCache_;
    std::vector<std::unique_ptr<IngressWorker>> workers_;
    std::vector<MPSCRingBuffer> ringBuffers_;
    std::vector<std::unique_ptr<RejectionBitset>> rejectedStateBuffers_;
    std::vector<std::unique_ptr<MatchingEngine>> engines_;
    std::vector<std::unique_ptr<Dispatcher>> dispatchers_;
    SymbolRouter router_;
    MarketState marketState_;

    struct EventBus
    {
        std::vector<EventSPSC<OrderEvent>> orderQueues;
        std::vector<EventSPSC<TradeEvent>> tradeQueues;
        std::vector<EventSPSC<BookUpdateEvent>> bookUpdateQueues;
        std::vector<EventSPSC<DBEvent>> ingressDbQueues;
        std::vector<EventSPSC<DBEvent>> engineDbQueues;
    } eventBus_;

    std::vector<EventSPSC<OrderEvent>> rejectOrderQueues_;
    const SymbolCache &symbolCache_;

public:
    EMSCore(size_t numWorkers, size_t numSymbols, const SymbolCache &symbolCache)
        : numWorkers_(numWorkers),
          numSymbols_(numSymbols),
          spscQueues_(),
          rateLimiter_(),
          balanceCache_(numSymbols),
          workers_(),
          ringBuffers_(),
          rejectedStateBuffers_(),
          engines_(),
          dispatchers_(),
          router_(numSymbols),
          marketState_(numSymbols),
          eventBus_(),
          rejectOrderQueues_(),
          symbolCache_(symbolCache)
    {
        spscQueues_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            spscQueues_.emplace_back(EMSConfig::SPSC_BUFFER_SIZE);
        }

        ringBuffers_.reserve(numSymbols_);
        rejectedStateBuffers_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            ringBuffers_.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
            rejectedStateBuffers_.push_back(std::make_unique<RejectionBitset>());
        }

        eventBus_.orderQueues.reserve(numSymbols_);
        eventBus_.tradeQueues.reserve(numSymbols_);
        eventBus_.bookUpdateQueues.reserve(numSymbols_);
        eventBus_.ingressDbQueues.reserve(numSymbols_);
        eventBus_.engineDbQueues.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            eventBus_.orderQueues.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
            eventBus_.tradeQueues.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
            eventBus_.bookUpdateQueues.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
            eventBus_.ingressDbQueues.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
            eventBus_.engineDbQueues.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
        }

        rejectOrderQueues_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            rejectOrderQueues_.emplace_back(EMSConfig::MPSC_BUFFER_SIZE);
        }

        engines_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            engines_.push_back(std::make_unique<MatchingEngine>(static_cast<Symbol>(i)));
            engines_.back()->setOutputQueues(
                static_cast<uint32_t>(i),
                eventBus_.orderQueues[i],
                eventBus_.tradeQueues[i],
                eventBus_.engineDbQueues[i],
                eventBus_.bookUpdateQueues[i]);
        }

        workers_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            workers_.push_back(std::make_unique<IngressWorker>(
                spscQueues_[i],
                rateLimiter_,
                balanceCache_,
                symbolCache_,
                router_,
                ringBuffers_,
                rejectedStateBuffers_,
                eventBus_.ingressDbQueues,
                &rejectOrderQueues_[i],
                i,
                marketState_));
        }

        dispatchers_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            dispatchers_.push_back(std::make_unique<Dispatcher>(
                static_cast<uint32_t>(i),
                ringBuffers_[i],
                *rejectedStateBuffers_[i],
                *engines_[i],
                EMSConfig::DISPATCHER_BASE_CORE + static_cast<int>(i)));
        }
    }

    [[nodiscard]] EventSPSC<OrderEvent> &orderQueue(size_t symbolIdx) noexcept
    {
        return eventBus_.orderQueues[symbolIdx];
    }

    [[nodiscard]] EventSPSC<TradeEvent> &tradeQueue(size_t symbolIdx) noexcept
    {
        return eventBus_.tradeQueues[symbolIdx];
    }

    [[nodiscard]] EventSPSC<DBEvent> &ingressDbQueue(size_t symbolIdx) noexcept
    {
        return eventBus_.ingressDbQueues[symbolIdx];
    }

    [[nodiscard]] EventSPSC<DBEvent> &engineDbQueue(size_t symbolIdx) noexcept
    {
        return eventBus_.engineDbQueues[symbolIdx];
    }

    [[nodiscard]] EventSPSC<OrderEvent> &rejectQueue(size_t workerIdx) noexcept
    {
        return rejectOrderQueues_[workerIdx];
    }

    [[nodiscard]] size_t numSymbols() const noexcept { return numSymbols_; }

    [[nodiscard]] std::vector<EventSPSC<TradeEvent>> &tradeQueues() noexcept
    {
        return eventBus_.tradeQueues;
    }

    [[nodiscard]] std::vector<EventSPSC<BookUpdateEvent>> &bookUpdateQueues() noexcept
    {
        return eventBus_.bookUpdateQueues;
    }

    [[nodiscard]] std::vector<EventSPSC<DBEvent>> &ingressDbQueues() noexcept
    {
        return eventBus_.ingressDbQueues;
    }

    [[nodiscard]] std::vector<EventSPSC<DBEvent>> &engineDbQueues() noexcept
    {
        return eventBus_.engineDbQueues;
    }

    [[nodiscard]] BalanceCache &balanceCache() noexcept { return balanceCache_; }

    [[nodiscard]] SPSCQueueWrapper &spscQueue(size_t workerIdx) noexcept
    {
        return spscQueues_[workerIdx];
    }

    [[nodiscard]] size_t numWorkers() const noexcept { return numWorkers_; }

    void start()
    {
        for (auto &w : workers_)
            w->start();
        for (auto &d : dispatchers_)
            d->start();
    }

    void stop()
    {
        for (auto &w : workers_)
            w->stop();
        for (auto &w : workers_)
            w->join();
        for (auto &d : dispatchers_)
            d->stop();
    }

    void join()
    {
        for (auto &d : dispatchers_)
            d->join();
    }
};
