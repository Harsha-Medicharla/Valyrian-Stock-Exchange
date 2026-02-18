#pragma once

#include <vector>
#include <memory>
#include <atomic>

#include "../queue/RingBuffer.h"
#include "../dispatcher/Dispatcher.h"
#include "../threading/IngressWorker.h"
#include "../pipeline/EMSPipeline.h"
#include "../model/OrderRequest.h"
class MatchingEngine; 

namespace EMS {

class EMS {
public:
    EMS(size_t symbol_count,
        EMSPipeline& pipeline); 

    void start();
    void stop();
    void submit(const model::OrderRequest& request);

private:
    size_t symbol_count_;
    EMSPipeline& pipeline_;

    std::vector<
        std::unique_ptr<
            RingBuffer<model::OrderRequest, 1024>
        >
    > queues_;

    std::vector<std::unique_ptr<MatchingEngine>> engines_;
    std::vector<std::unique_ptr<Dispatcher>> dispatchers_;
    std::vector<std::unique_ptr<IngressWorker>> ingress_workers_;

    std::atomic<size_t> next_worker_{0};
};

}