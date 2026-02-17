#include "Dispatcher.h"
#include <pthread.h>

Dispatcher::Dispatcher(RingBuffer<EMS::model::OrderRequest, 1024>& queue,
                       MatchingEngine& engine)
    : queue_(queue), engine_(engine) {}

void Dispatcher::start() {
    running_.store(true, std::memory_order_release);
    worker_ = std::thread(&Dispatcher::run, this);
}

void Dispatcher::stop() {
    running_.store(false, std::memory_order_release);
}

void Dispatcher::join() {
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Dispatcher::run() {

    // Optional: pin to core 2 (example)
    pinThreadToCore(2);

    EMS::model::OrderRequest req;

    while (true) {

        // Fast path: process as many as possible
        while (queue_.pop(req)) {
            engine_.onNewOrder(req);
        }

        if (!running_.load(std::memory_order_acquire))
            break;

        // Spin pause
        _mm_pause();
    }
}

void Dispatcher::pinThreadToCore(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
