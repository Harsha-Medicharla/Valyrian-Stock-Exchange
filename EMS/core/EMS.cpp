#include "EMS.h"
#include "../../MatchingEngine/include/MatchingEngine.h"

namespace EMS {

EMS::EMS(size_t symbol_count,
         EMSPipeline& pipeline)
    : symbol_count_(symbol_count),
      pipeline_(pipeline)
{
    engines_.reserve(symbol_count_);
    dispatchers_.reserve(symbol_count_);
    queues_.reserve(symbol_count_);

    // 1️⃣ Create per-symbol queues, engines, dispatchers
    for (size_t i = 0; i < symbol_count_; ++i) {

        auto queue = std::make_unique<
            RingBuffer<model::OrderRequest, 1024>
        >();

        auto engine = std::make_unique<MatchingEngine>(i);

        auto dispatcher = std::make_unique<Dispatcher>(
            *queue,
            *engine
        );

        queues_.push_back(std::move(queue));
        engines_.push_back(std::move(engine));
        dispatchers_.push_back(std::move(dispatcher));
    }

    // 2️⃣ Create ingress workers
    size_t ingress_count = std::thread::hardware_concurrency();
    if (ingress_count == 0)
        ingress_count = 4;

    ingress_workers_.reserve(ingress_count);

    for (size_t i = 0; i < ingress_count; ++i) {
        ingress_workers_.push_back(
            std::make_unique<IngressWorker>(
                pipeline_,
                queues_
            )
        );
    }
}

void EMS::start()
{
    // Start ingress workers first
    for (auto& worker : ingress_workers_)
        worker->start();

    // Start dispatchers
    for (auto& dispatcher : dispatchers_)
        dispatcher->start();
}

void EMS::stop()
{
    // Stop ingress workers
    for (auto& worker : ingress_workers_)
        worker->stop();

    for (auto& worker : ingress_workers_)
        worker->join();

    // Stop dispatchers
    for (auto& dispatcher : dispatchers_)
        dispatcher->stop();

    for (auto& dispatcher : dispatchers_)
        dispatcher->join();
}

void EMS::submit(const model::OrderRequest& request)
{
    size_t index = next_worker_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    index %= ingress_workers_.size();

    ingress_workers_[index]->submit(request);
}

} // namespace EMS
