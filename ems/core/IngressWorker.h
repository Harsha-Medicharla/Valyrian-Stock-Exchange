#pragma once
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "../pipeline/MarketState.h"
#include "../pipeline/RateLimiter.h"
#include "../pipeline/ValidationPipeline.h"
#include "../routing/SymbolRouter.h"
#include "RejectHandler.h"
#include "../utils/SpinWait.h"

class IngressWorker
{
private:
    SPSCQueueWrapper &queue_;
    RateLimiter rateLimiter_;
    ValidationPipeline pipeline_;
    SymbolRouter &router_;
    std::vector<MPSCRingBuffer> &ringBuffers_;
    std::thread thread_;
    std::atomic<bool> running_;
    uint64_t opsSinceDecay_;

    void run()
    {
        // Decay the rate limiter periodically. This is "time-ish" and cheap:
        // it is tied to the number of processed events.
        static constexpr uint64_t kDecayEveryOps = 1u << 16;

        while (running_.load(std::memory_order_relaxed))
        {
            RawOrder *slot = nullptr;

            if (queue_.pop(slot))
            {
                RejectReason reason;
                const Decision decision = pipeline_.process(slot, reason);

                if (decision == Decision::REJECT)
                {
                    RejectHandler::invoke(slot->user_id, reason);
                    queue_.releaseSlot();
                }
                else
                {
                    const size_t idx = router_.route(slot->symbol_id);
                    MPSCRingBuffer &rb = ringBuffers_[idx];

                    uint64_t seq = 0;
                    OrderSlot *out = rb.claimSlot(seq);

                    // Copy RawOrder -> OrderSlot (no heap, no partial writes after publish).
                    out->sequence = seq;
                    out->user_id = slot->user_id;
                    out->symbol_id = slot->symbol_id;
                    out->price = slot->price;
                    out->qty = slot->qty;
                    out->side = slot->side;
                    out->type = slot->type;

                    queue_.releaseSlot();
                    rb.publish(seq);
                }

                // Periodic decay to allow the limiter to recover.
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
        SymbolRouter &router,
        std::vector<MPSCRingBuffer> &ringBuffers,
        MarketState &marketState) noexcept
        : queue_(queue),
          rateLimiter_(),
          pipeline_(rateLimiter_, marketState),
          router_(router),
          ringBuffers_(ringBuffers),
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
