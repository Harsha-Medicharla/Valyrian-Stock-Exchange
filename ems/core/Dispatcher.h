#pragma once
#include <thread>
#include <atomic>
#include "../queues/MPSCRingBuffer.h"
#include <pthread.h>
#include <sched.h>
#include "../utils/SpinWait.h"
#include "MatchingEngine.h"

class Dispatcher
{
private:
    uint32_t symbolId_;
    MPSCRingBuffer &ring_;
    MatchingEngine &engine_;
    int coreId_;
    std::thread thread_;
    std::atomic<bool> running_;
    uint64_t nextSequence_;

    void run()
    {
        pinToCore();

        while (running_.load(std::memory_order_relaxed))
        {

            uint64_t cursor = ring_.cursor();
            uint64_t highest = ring_.getHighestPublished(nextSequence_, cursor);

            if (highest >= nextSequence_)
            {

                for (uint64_t seq = nextSequence_; seq <= highest; seq++)
                {

                    OrderSlot &slot = ring_.get(seq);

                    engine_.onNewOrder(
                        slot.symbol_id,
                        slot.price,
                        slot.qty,
                        slot.side,
                        slot.type,
                        slot.user_id,
                        slot.sequence);

                    ring_.releaseSlot(seq);
                }

                nextSequence_ = highest + 1;
            }
            else
            {
                ems::pause();
            }
        }
    }

    void pinToCore()
    {
// pthread affinity is Linux-specific; keep dispatcher portable.
#if defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coreId_, &cpuset);

        pthread_setaffinity_np(
            pthread_self(),
            sizeof(cpu_set_t),
            &cpuset);
#else
        (void)coreId_;
#endif
    }

public:
    inline Dispatcher(uint32_t symbolId, MPSCRingBuffer &ring, MatchingEngine &engine, int coreId)
        : symbolId_(symbolId),
          ring_(ring),
          engine_(engine),
          coreId_(coreId),
          thread_(),
          running_(false),
          nextSequence_(1)
    {
    }

    inline void start()
    {
        running_.store(true, std::memory_order_release);
        thread_ = std::thread(&Dispatcher::run, this);
    }

    inline void stop()
    {
        running_.store(false, std::memory_order_release);
    }

    inline void join()
    {
        if (thread_.joinable())
            thread_.join();
    }
};
