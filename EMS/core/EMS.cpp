#include "core/EMS.h"
#include "pipeline/EMSPipeline.h"
#include "threading/IngressWorker.h"
#include "dispatcher/Dispatcher.h" 
#include "ports/EgressPort.h"
#include "../../MatchingEngine/include/MatchingEngine.h"
#include "queue/RingBuffer.h"
#include "SettlementModule.h" // 1. ADD THIS INCLUDE

namespace EMS {

// 2. Update constructor signature and initialize bank_
EMSCore::EMSCore(size_t symbol_count, 
                 EMSPipeline& pipeline, 
                 EgressPort& egress,
                 Settlement::SettlementModule& bank)
    : symbol_count_(symbol_count), 
      pipeline_(pipeline), 
      egress_(egress),
      bank_(bank) // <-- Initialize the reference
{
    engines_.reserve(symbol_count_);
    dispatchers_.reserve(symbol_count_);
    queues_.reserve(symbol_count_);

    for (size_t i = 0; i < symbol_count_; ++i) {
        auto queue = std::make_unique<RingBuffer<model::OrderRequest, 1024>>();
        
        // 3. THE FINAL BOSS DEFEATED: Hand the bank to the engine
        auto engine = std::make_unique<MatchingEngine>(i, bank_); 
        
        auto dispatcher = std::make_unique<Dispatcher>(*queue, *engine);

        queues_.push_back(std::move(queue));
        engines_.push_back(std::move(engine));
        dispatchers_.push_back(std::move(dispatcher));
    }

    size_t ingress_count = std::thread::hardware_concurrency();
    if (ingress_count == 0) ingress_count = 4;

    ingress_workers_.reserve(ingress_count);

    for (size_t i = 0; i < ingress_count; ++i) {
        ingress_workers_.push_back(
            std::make_unique<IngressWorker>(*this)
        );
    }
}

void EMSCore::start() {
    for (auto& worker : ingress_workers_) worker->start();
    for (auto& dispatcher : dispatchers_) dispatcher->start();
}

void EMSCore::stop() {
    for (auto& worker : ingress_workers_) worker->stop();
    for (auto& worker : ingress_workers_) worker->join();
    for (auto& dispatcher : dispatchers_) dispatcher->stop();
    for (auto& dispatcher : dispatchers_) dispatcher->join();
}

void EMSCore::submit(const model::OrderRequest& request) {
    model::EMSDecision decision = pipeline_.process(request);
    egress_.forward(decision);
    if (decision.accepted) {
        size_t symbol_idx = static_cast<size_t>(request.symbol);
        
        if (symbol_idx < queues_.size()) {
            queues_[symbol_idx]->push(request);
        }
    }
}

}