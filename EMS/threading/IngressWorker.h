#pragma once

#include "../model/OrderRequest.h"
#include "../pipeline/EMSPipeline.h"
#include "../queue/RingBuffer.h"
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

namespace EMS {

class IngressWorker {
public:
    IngressWorker(
        EMSPipeline &pipeline,
        std::vector<std::unique_ptr<RingBuffer<model::OrderRequest, 1024>>>
            &symbol_queues);

    void start();
    void stop();
    void join();
    void submit(const model::OrderRequest &request);

private:
    void run();

    EMSPipeline &pipeline_;
    std::vector<std::unique_ptr<RingBuffer<model::OrderRequest, 1024>>>
        &symbol_queues_;

    RingBuffer<model::OrderRequest, 2048> ingress_queue_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};

} 