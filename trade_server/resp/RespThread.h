#pragma once
#include <atomic>
#include <cstdint>
#include <thread>

#include "core/EMSCore.h"
#include "core/IngressWorker.h"
#include "core/RejectHandler.h"
#include "types/RawOrder.h"
#include "shared/types/Events.h"
#include "network/WebSocketServer.h"
#include "utils/SpinWait.h"

class RespThread
{
private:
    EMSCore &ems_;
    WebSocketServer &ws_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    static void rejectCallback(const RawOrder *order, RejectReason reason) noexcept
    {
        if (!order || !tls_rejectQueue)
            return;
        OrderEvent ev{};
        ev.type = OrderEventType::REJECT;
        ev.order_id = order->order_id;
        ev.user_id = static_cast<UserId>(order->user_id);
        ev.symbol_id = order->symbol_id;
        ev.fill_price = 0;
        ev.fill_qty = 0;
        ev.remaining = static_cast<Qty>(order->qty);
        ev.state = OrderState::NEW;
        ev.reject_reason = reason;
        ev.sequence = static_cast<SeqNo>(order->sequence);
        ev.timestamp = order->timestamp;
        (void)tls_rejectQueue->tryPush(ev);
    }

    void run()
    {
        while (running_.load(std::memory_order_relaxed))
        {
            bool any = false;
            for (std::size_t s = 0; s < ems_.numSymbols(); ++s)
            {
                while (OrderEvent *ev = ems_.orderQueue(s).front())
                {
                    (void)ev;
                    ems_.orderQueue(s).pop();
                    any = true;
                }
            }
            for (std::size_t w = 0; w < ems_.numWorkers(); ++w)
            {
                while (OrderEvent *ev = ems_.rejectQueue(w).front())
                {
                    (void)ev;
                    ems_.rejectQueue(w).pop();
                    any = true;
                }
            }
            if (!any)
                ems::pause();
            (void)ws_;
        }
    }

public:
    RespThread(EMSCore &ems, WebSocketServer &ws) noexcept : ems_(ems), ws_(ws) {}

    void start()
    {
        RejectHandler::registerCallback(&rejectCallback);
        running_.store(true, std::memory_order_release);
        thread_ = std::thread(&RespThread::run, this);
    }

    void stop() noexcept { running_.store(false, std::memory_order_release); }

    void join()
    {
        if (thread_.joinable())
            thread_.join();
    }
};
