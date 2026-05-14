#pragma once
#include <thread>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "../queues/MPSCRingBuffer.h"
#include "../queues/RejectionBitset.h"
#include "../config/EMSConfig.h"
#include <pthread.h>
#include <sched.h>
#include "../utils/SpinWait.h"
#include "../types/OrderSlot.h"
#include "../../MatchingEngine/include/MatchingEngine.h"

class Dispatcher
{
private:
    [[maybe_unused]] uint32_t symbolId_;
    MPSCRingBuffer &ring_;
    RejectionBitset &rejectedStates_;
    MatchingEngine &engine_;
    int coreId_;
    std::thread thread_;
    std::atomic<bool> running_;
    uint64_t nextServerSequence_;

    static constexpr uint64_t kMask = static_cast<uint64_t>(EMSConfig::MPSC_BUFFER_SIZE - 1);

    [[nodiscard]] inline uint64_t mapServerToRingSeq(uint64_t serverSeq) const noexcept
    {
        // If serverSeq starts at 1, but ring is 0-indexed:
        return (serverSeq - 1) & kMask;
    }

    void run() noexcept
    {
        pinToCore();

        while (running_.load(std::memory_order_relaxed))
        {
            if (rejectedStates_.tryConsumeRejected(nextServerSequence_))
            {
                ++nextServerSequence_;
                continue;
            }

            const uint64_t ringSeq = mapServerToRingSeq(nextServerSequence_);
            if (ring_.isAvailable(ringSeq))
            {
                OrderSlot &slot = ring_.get(ringSeq);
                if (slot.sequence != nextServerSequence_)
                {
                    ems::pause();
                    continue;
                }

                if (slot.cancel_flag != 0)
                {
                    engine_.onCancelOrder(slot.order_id);
                }
                else if (slot.modify_flag != 0)
                {
                    engine_.onModifyOrder(slot.order_id, static_cast<Price>(slot.price),
                                          static_cast<Qty>(slot.qty));
                }
                else
                {
                    engine_.onNewOrder(
                        slot.order_id,
                        static_cast<UserId>(slot.user_id),
                        static_cast<Side>(slot.side),
                        static_cast<OrderType>(slot.type),
                        static_cast<Price>(slot.price),
                        static_cast<Qty>(slot.qty),
                        static_cast<TimeStamp>(slot.timestamp));
                }
                ring_.releaseSlot(ringSeq);
                ++nextServerSequence_;
                continue;
            }

            ems::pause();
        }
    }

    void pinToCore() noexcept
    {
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
    inline Dispatcher(
        uint32_t symbolId,
        MPSCRingBuffer &ring,
        RejectionBitset &rejectedStates,
        MatchingEngine &engine,
        int coreId) noexcept
        : symbolId_(symbolId),
          ring_(ring),
          rejectedStates_(rejectedStates),
          engine_(engine),
          coreId_(coreId),
          thread_(),
          running_(false),
          nextServerSequence_(1)
    {
    }

    inline void start() noexcept
    {
        running_.store(true, std::memory_order_release);
        thread_ = std::thread(&Dispatcher::run, this);
    }

    inline void stop() noexcept
    {
        running_.store(false, std::memory_order_release);
    }

    inline void join() noexcept
    {
        if (thread_.joinable())
            thread_.join();
    }
};
