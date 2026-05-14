#pragma once
#include <thread>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include "../queues/MPSCRingBuffer.h"
#include "../queues/RejectionBitset.h"
#include "../config/EMSConfig.h"
#include "../../trade_server/ThreadAffinity.h"
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
    uint64_t nextRingSequence_;

    void run() noexcept
    {
        pinToCore();

        while (running_.load(std::memory_order_relaxed))
        {
            // 1. Check if the current server sequence was rejected pre-trade
            if (rejectedStates_.tryConsumeRejected(nextServerSequence_))
            {
                ++nextServerSequence_;
                continue;
            }

            // 2. Otherwise, it must be waiting for us in the next physical ring slot
            if (ring_.isAvailable(nextRingSequence_))
            {
                OrderSlot &slot = ring_.get(nextRingSequence_);
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
                ring_.releaseSlot(nextRingSequence_);
                ++nextRingSequence_;
                ++nextServerSequence_;
                continue;
            }

            ems::pause();
        }
    }

    void pinToCore() noexcept
    {
        if (const auto mapped = vse::threads::coreForRole("METhread " + std::to_string(symbolId_)))
            vse::threads::pinToCore(*mapped);
        else
            vse::threads::pinToCore(coreId_);
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
          nextServerSequence_(1),
          nextRingSequence_(1)
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
