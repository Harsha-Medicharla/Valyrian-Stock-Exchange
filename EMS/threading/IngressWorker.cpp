#include "IngressWorker.h"
#include <immintrin.h>
namespace EMS {

IngressWorker::IngressWorker(
    EMSPipeline &pipeline,
    std::vector<std::unique_ptr<RingBuffer<model::OrderRequest, 1024>>>
        &symbol_queues)
    : pipeline_(pipeline), symbol_queues_(symbol_queues) {}

void IngressWorker::start() {
  running_.store(true, std::memory_order_release);
  worker_ = std::thread(&IngressWorker::run, this);
}

void IngressWorker::stop() { running_.store(false, std::memory_order_release); }

void IngressWorker::join() {
  if (worker_.joinable())
    worker_.join();
}

void IngressWorker::submit(const model::OrderRequest &request) {
  ingress_queue_.push(request);
}

void IngressWorker::run() {

  model::OrderRequest req;

  while (running_.load(std::memory_order_acquire)) {

    while (ingress_queue_.pop(req)) {

      auto decision = pipeline_.process(req);

      if (decision.accepted) {
        symbol_queues_[decision.route_index]->push(req);
      }
    }

    _mm_pause();
  }
}

} // namespace EMS
