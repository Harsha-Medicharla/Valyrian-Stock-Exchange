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

inline thread_local EventSPSC<OrderEvent> *tls_rejectQueue = nullptr;

class IngressWorker
{
private:
    SPSCQueueWrapper &queue_;
    RateLimiter &rateLimiter_;
    ValidationPipeline pipeline_;
    SymbolRouter &router_;
    std::vector<MPSCRingBuffer> &ringBuffers_;
    std::vector<std::unique_ptr<RejectionBitset>> &rejectedStateBuffers_;
    std::vector<EventSPSC<DBEvent>> &dbQueues_;
    EventSPSC<OrderEvent> *rejectQueue_;
    std::size_t workerIdx_;
    std::thread thread_;
    std::atomic<bool> running_;

    void run()
    {
        tls_rejectQueue = rejectQueue_;
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
                        uint64_t ringSeq = 0;
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

                        if (idx < dbQueues_.size())
                        {
                            DBEvent de{};
                            de.wal_sequence = static_cast<SeqNo>(slot->sequence);
                            de.type = DBEventType::ORDER_ACCEPTED;
                            de.side = static_cast<Side>(slot->side);
                            de.order_type = static_cast<OrderType>(slot->type);
                            de.state = OrderState::NEW;
                            de.order_id = slot->order_id;
                            de.user_id = static_cast<UserId>(slot->user_id);
                            de.symbol_id = slot->symbol_id;
                            de.price = slot->price;
                            de.qty = static_cast<Qty>(slot->qty);
                            de.remaining = static_cast<Qty>(slot->qty);
                            de.timestamp = slot->timestamp;
                            de.peer_order_id = 0;
                            de.peer_user_id = 0;
                            (void)dbQueues_[idx].tryPush(de);
                        }
                    }

                    queue_.releaseSlot();
                }
            }
            else
            {
                ems::pause();
            }
        }

        tls_rejectQueue = nullptr;
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
        std::vector<EventSPSC<DBEvent>> &dbQueues,
        EventSPSC<OrderEvent> *rejectQueue,
        std::size_t workerIdx,
        MarketState &marketState) noexcept
        : queue_(queue),
          rateLimiter_(rateLimiter),
          pipeline_(rateLimiter_, marketState, balanceCache, symbolCache),
          router_(router),
          ringBuffers_(ringBuffers),
          rejectedStateBuffers_(rejectedStateBuffers),
          dbQueues_(dbQueues),
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
