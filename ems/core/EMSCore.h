#pragma once
#include <vector>
#include <memory>
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "IngressWorker.h"
#include "Dispatcher.h"
#include "../routing/SymbolRouter.h"
#include "../pipeline/MarketState.h"
#include "MatchingEngine.h"

class EMSCore
{
private:
    size_t numWorkers_;
    size_t numSymbols_;
    std::vector<SPSCQueueWrapper> spscQueues_;
    std::vector<std::unique_ptr<IngressWorker>> workers_;
    std::vector<MPSCRingBuffer> ringBuffers_;
    std::vector<MatchingEngine> engines_;
    std::vector<std::unique_ptr<Dispatcher>> dispatchers_;
    SymbolRouter router_;
    MarketState marketState_;

public:
    static constexpr size_t SPSC_BUFFER_SIZE = 1u << 12; // power-of-two
    static constexpr size_t MPSC_BUFFER_SIZE = 1u << 12; // power-of-two
    static constexpr int DISPATCHER_BASE_CORE = 2;

    EMSCore(size_t numWorkers, size_t numSymbols)
        : numWorkers_(numWorkers),
          numSymbols_(numSymbols),
          router_(numSymbols),
          marketState_(numSymbols),
          spscQueues_(),
          ringBuffers_(),
          workers_(),
          engines_(),
          dispatchers_()
    {
        spscQueues_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            spscQueues_.emplace_back(SPSC_BUFFER_SIZE);
        }

        ringBuffers_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            ringBuffers_.emplace_back(MPSC_BUFFER_SIZE);
        }

        engines_.resize(numSymbols_);

        workers_.reserve(numWorkers_);
        for (size_t i = 0; i < numWorkers_; ++i)
        {
            workers_.push_back(std::make_unique<IngressWorker>(
                spscQueues_[i],
                router_,
                ringBuffers_,
                marketState_));
        }

        dispatchers_.reserve(numSymbols_);
        for (size_t i = 0; i < numSymbols_; ++i)
        {
            dispatchers_.push_back(std::make_unique<Dispatcher>(
                static_cast<uint32_t>(i),
                ringBuffers_[i],
                engines_[i],
                DISPATCHER_BASE_CORE + static_cast<int>(i)));
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
