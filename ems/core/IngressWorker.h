#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "shared/types/Events.h"
#include "shared/queues/EventSPSC.h"
#include "../queues/SPSCQueueWrapper.h"
#include "../queues/MPSCRingBuffer.h"
#include "../queues/RejectionBitset.h"
#include "../pipeline/BalanceCache.h"
#include "../pipeline/MarketState.h"
#include "../pipeline/RateLimiter.h"
#include "../pipeline/ValidationPipeline.h"
#include "../../trade_server/ThreadAffinity.h"
#include "../routing/SymbolRouter.h"
#include "../types/RawOrder.h"
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
    std::vector<std::unique_ptr<RejectionBitset>> &rejectedStateBuffers_;
    EventSPSC<OrderEvent> *rejectQueue_;
    std::size_t workerIdx_;
    std::thread thread_;
    std::atomic<bool> running_;

    void run()
    {
        RejectHandler::setRejectQueue(rejectQueue_);
        if (const auto core = vse::threads::coreForRole("IOThread " + std::to_string(workerIdx_)))
            vse::threads::pinToCore(*core);

        while (running_.load(std::memory_order_relaxed))
        {
            RawOrder *slot = nullptr;

            if (queue_.pop(slot))
            {
                if (slot->sequence == 0)
                {
                    RejectHandler::invoke(slot, RejectReason::INVALID_RANGE);
                    queue_.releaseSlot();
                }
                else
                {
                    const size_t idx = router_.route(slot->symbol_id);

                    RejectReason reason;
                    const Decision decision = pipeline_.process(slot, reason);

                    if (decision == Decision::REJECT)
                    {
                        RejectHandler::invoke(slot, reason);
                        rejectedStateBuffers_[idx]->markRejected(slot->sequence);
                    }
                    else
                    {
                        MPSCRingBuffer &rb = ringBuffers_[idx];
                        uint64_t ringSeq = slot->sequence;
                        OrderSlot *out = rb.claimSlot(ringSeq);
                        out->sequence = slot->sequence;
                        out->order_id = slot->order_id;
                        out->timestamp = slot->timestamp;
                        out->user_id = slot->user_id;
                        out->symbol_id = slot->symbol_id;
                        out->price = slot->price;
                        out->qty = slot->qty;
                        out->side = slot->side;
                        out->type = slot->type;
                        out->cancel_flag = slot->cancel_flag;
                        out->modify_flag = slot->modify_flag;
                        rb.publish(ringSeq);
                    }

                    queue_.releaseSlot();
                }
            }
            else
            {
                ems::pause();
            }
        }

        RejectHandler::setRejectQueue(nullptr);
    }

public:
    IngressWorker(
        SPSCQueueWrapper &queue,
        RateLimiter &rateLimiter,
        BalanceCache &balanceCache,
        const SymbolCache &symbolCache,
        SymbolRouter &router,
        std::vector<MPSCRingBuffer> &ringBuffers,
        std::vector<std::unique_ptr<RejectionBitset>> &rejectedStateBuffers,
        EventSPSC<OrderEvent> *rejectQueue,
        std::size_t workerIdx,
        MarketState &marketState) noexcept
        : queue_(queue),
          rateLimiter_(rateLimiter),
          pipeline_(rateLimiter_, marketState, balanceCache, symbolCache),
          router_(router),
          ringBuffers_(ringBuffers),
          rejectedStateBuffers_(rejectedStateBuffers),
          rejectQueue_(rejectQueue),
          workerIdx_(workerIdx),
          thread_(),
          running_(false)
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
