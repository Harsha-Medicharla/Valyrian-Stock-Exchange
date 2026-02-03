#pragma once
#include <vector>
#include <memory>
#include "../config/EMSConfig.h"
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "../queues/SequenceStateBuffer.h"
#include "IngressWorker.h"
#include "Dispatcher.h"
#include "../routing/SymbolRouter.h"
#include "../pipeline/MarketState.h"
#include "../pipeline/RateLimiter.h"
#include "MatchingEngine.h"

class EMSCore
{
private:
    size_t numWorkers_;
    size_t numSymbols_;
    std::vector<SPSCQueueWrapper> spscQueues_;
    RateLimiter rateLimiter_;
    std::vector<std::unique_ptr<IngressWorker>> workers_;
    std::vector<MPSCRingBuffer> ringBuffers_;
    std::vector<std::unique_ptr<SequenceStateBuffer>> rejectedStateBuffers_;
    std::vector<std::unique_ptr<MatchingEngine>> engines_;
    std::vector<std::unique_ptr<Dispatcher>> dispatchers_;  
    SymbolRouter router_;
    MarketState marketState_;

public:
    EMSCore(size_t numWorkers, size_t numSymbols)
        : numWorkers_(numWorkers),
          numSymbols_(numSymbols),
          router_(numSymbols),
          marketState_(numSymbols),
          spscQueues_(),
          rateLimiter_(),
          ringBuffers_(),
          rejectedStateBuffers_(),
          workers_(),
          engines_(),
          dispatchers_()
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
            rejectedStateBuffers_.push_back(std::make_unique<SequenceStateBuffer>());
        }

        engines_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            engines_.push_back(std::make_unique<MatchingEngine>());
        }

        workers_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            workers_.push_back(std::make_unique<IngressWorker>(
                spscQueues_[i],
                rateLimiter_,
                router_,
                ringBuffers_,
                rejectedStateBuffers_,
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
        for (auto &d : dispatchers_)
            d->stop();
    }

    void join()
    {
        for (auto &w : workers_)
            w->join();
        for (auto &d : dispatchers_)
            d->join();
    }
};
