#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <cstddef>
#include "EMS/model/OrderRequest.h"

// Forward declaration for global Settlement class
namespace SettlementCore {
    class Settlement;
}

// Forward declarations for EMS-specific components
namespace EMS {
    class IngressWorker;
    class EMSPipeline;
    class Dispatcher;
    class EgressPort;
}

// Forward declarations for external dependencies
class MatchingEngine;
template<typename T, size_t Size> class RingBuffer;

namespace EMS {

class EMSCore {
public:
    /**
     * @brief Constructs the core Execution Management System.
     * @param symbol_count Number of trading symbols to manage.
     * @param pipeline Reference to the order validation pipeline.
     * @param egress Reference to the output port.
     * @param bank Reference to the global Settlement module.
     */
    EMSCore(size_t symbol_count, 
            EMS::EMSPipeline& pipeline, 
            EMS::EgressPort& egress, 
            SettlementCore::Settlement& bank);
    
    void start();
    void stop();
    void submit(const model::OrderRequest& request);

private:
    size_t symbol_count_;
    EMS::EMSPipeline& pipeline_;
    EMS::EgressPort& egress_;
    SettlementCore::Settlement& bank_;

    // Internal components using full namespacing/paths
    std::vector<std::unique_ptr<RingBuffer<model::OrderRequest, 1024>>> queues_;
    std::vector<std::unique_ptr<MatchingEngine>> engines_;
    std::vector<std::unique_ptr<Dispatcher>> dispatchers_;
    std::vector<std::unique_ptr<IngressWorker>> ingress_workers_;
    
    std::atomic<size_t> next_worker_{0};
};

} // namespace EMS