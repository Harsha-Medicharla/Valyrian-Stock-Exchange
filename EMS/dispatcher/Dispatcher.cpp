#include "Dispatcher.h"
#include "../../MatchingEngine/include/MatchingEngine.h"
#include <pthread.h>
#include <immintrin.h>

namespace EMS {

Dispatcher::Dispatcher(RingBuffer<model::OrderRequest, 1024>& queue,
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
    pinThreadToCore(2);

    model::OrderRequest req;

    while (true) {
        while (queue_.pop(req)) {
            engine_.onNewOrder(req.order_id, req.user_id, req.side, req.type, 
                               req.price, req.quantity, req.wall_time_ns);
        }

        if (!running_.load(std::memory_order_acquire))
            break;
        _mm_pause();
    }
}

void Dispatcher::pinThreadToCore(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(worker_.native_handle(), sizeof(cpu_set_t), &cpuset);
}

} 