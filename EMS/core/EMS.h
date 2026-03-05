#pragma once
#include <vector>
#include <memory>
#include <atomic>
#include "../model/OrderRequest.h"

namespace EMS {
    // NO INCLUDES for internal components here!
    class IngressWorker;
    class EMSPipeline;
    class Dispatcher;
    class EgressPort;
}
class MatchingEngine;
template<typename T, size_t Size> class RingBuffer;

namespace EMS {
    class EMSCore {
    public:
        EMSCore(size_t symbol_count, EMSPipeline& pipeline, EgressPort& egress);
        void start();
        void stop();
        void submit(const model::OrderRequest& request);
    private:
        size_t symbol_count_;
        EMSPipeline& pipeline_;
        EgressPort& egress_;
        std::vector<std::unique_ptr<RingBuffer<model::OrderRequest, 1024>>> queues_;
        std::vector<std::unique_ptr<MatchingEngine>> engines_;
        std::vector<std::unique_ptr<Dispatcher>> dispatchers_;
        std::vector<std::unique_ptr<IngressWorker>> ingress_workers_;
        std::atomic<size_t> next_worker_{0};
    };
}