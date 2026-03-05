#include "IngressWorker.h"
#include <immintrin.h>

namespace EMS {

IngressWorker::IngressWorker(EMSCore& ems_core)
    : ems_core_(ems_core) {}

void IngressWorker::start() {
    running_.store(true, std::memory_order_release);
    worker_ = std::thread(&IngressWorker::run, this);
}

void IngressWorker::stop() { 
    running_.store(false, std::memory_order_release); 
}

void IngressWorker::join() {
    if (worker_.joinable())
        worker_.join();
}

void IngressWorker::submit(const model::OrderRequest& request) {
    ingress_queue_.push(request);
}

void IngressWorker::run() {
    model::OrderRequest req;

    while (running_.load(std::memory_order_acquire)) {
        while (ingress_queue_.pop(req)) {
            ems_core_.submit(req);
        }
        _mm_pause();
    }
}

}