#pragma once

#include "../model/OrderRequest.h"
#include "../core/EMS.h"
#include "../queue/RingBuffer.h"
#include <atomic>
#include <thread>

namespace EMS {

class IngressWorker {
public:
    explicit IngressWorker(EMSCore& ems_core);

    void start();
    void stop();
    void join();
    void submit(const model::OrderRequest& request);

private:
    void run();

    EMSCore& ems_core_;
    RingBuffer<model::OrderRequest, 2048> ingress_queue_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

}