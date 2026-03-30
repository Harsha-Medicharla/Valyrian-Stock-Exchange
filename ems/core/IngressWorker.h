#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "../queues/SequenceStateBuffer.h"
#include "../pipeline/MarketState.h"
#include "../pipeline/RateLimiter.h"
#include "../pipeline/ValidationPipeline.h"
#include "../routing/SymbolRouter.h"
#include "../types/OrderSlot.h"
#include "RejectHandler.h"
#include "../utils/SpinWait.h"

class IngressWorker
{
private:
    SPSCQueueWrapper &queue_;
    RateLimiter &rateLimiter_;
    ValidationPipeline pipeline_;
    SymbolRouter &router_;
    std::vector<MPSCRingBuffer> &ringBuffers_;
    std::vector<std::unique_ptr<SequenceStateBuffer>> &rejectedStateBuffers_;
    MarketState &marketState_;
    std::thread thread_;
    std::atomic<bool> running_;
    uint64_t opsSinceDecay_;

    void run()
    {
        static constexpr uint64_t kDecayEveryOps = 1u << 16;

        while (running_.load(std::memory_order_relaxed))
        {
            RawOrder *slot = nullptr;

            if (queue_.pop(slot))
            {
                if (slot->sequence == 0)
                {
                    RejectHandler::invoke(slot->user_id, RejectReason::INVALID_RANGE);
                    queue_.releaseSlot();
                }
                else if (slot->symbol_id >= marketState_.symbolCount())
                {
                    RejectHandler::invoke(slot->user_id, RejectReason::INVALID_RANGE);
                    queue_.releaseSlot();
                }
                else
                {
                    const size_t idx = router_.route(slot->symbol_id);

                    RejectReason reason;
                    const Decision decision = pipeline_.process(slot, reason);

                    if (decision == Decision::REJECT)
                    {
                        RejectHandler::invoke(slot->user_id, reason);
                        rejectedStateBuffers_[idx]->markRejected(slot->sequence);
                    }
                    else
                    {
                        MPSCRingBuffer &rb = ringBuffers_[idx];
                        uint64_t ringSeq = 0;
                        OrderSlot *out = rb.claimSlot(ringSeq);
                        out->sequence = slot->sequence;
                        out->state = OrderSlotState::Valid;
                        out->order_id = slot->order_id;
                        out->timestamp = slot->timestamp;
                        out->user_id = slot->user_id;
                        out->symbol_id = slot->symbol_id;
                        out->price = slot->price;
                        out->qty = slot->qty;
                        out->side = slot->side;
                        out->type = slot->type;
                        rb.publish(ringSeq);
                    }

                    queue_.releaseSlot();
                }

                if (++opsSinceDecay_ >= kDecayEveryOps)
                {
                    pipeline_.decay();
                    opsSinceDecay_ = 0;
                }
            }
            else
            {
                ems::pause();
            }
        }
    }

public:
    IngressWorker(
        SPSCQueueWrapper &queue,
        RateLimiter &rateLimiter,
        SymbolRouter &router,
        std::vector<MPSCRingBuffer> &ringBuffers,
        std::vector<std::unique_ptr<SequenceStateBuffer>> &rejectedStateBuffers,
        MarketState &marketState) noexcept
        : queue_(queue),
          rateLimiter_(rateLimiter),
          pipeline_(rateLimiter_, marketState),
          router_(router),
          ringBuffers_(ringBuffers),
          rejectedStateBuffers_(rejectedStateBuffers),
          marketState_(marketState),
          thread_(),
          running_(false),
          opsSinceDecay_(0)
    {
    }

    void start()
    {
        running_.store(true, std::memory_order_release);
        thread_ = std::thread(&IngressWorker::run, this);
    }

    void stop()
    {
        running_.store(false, std::memory_order_release);
    }

    void join()
    {
        if (thread_.joinable())
            thread_.join();
    }
};
