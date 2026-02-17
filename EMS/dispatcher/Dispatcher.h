#pragma once

#include <atomic>
#include <thread>
#include <immintrin.h>
#include "../queue/RingBuffer.h"
#include "../model/OrderRequest.h"


class MatchingEngine;

class Dispatcher {
public:
    Dispatcher(RingBuffer<EMS::model::OrderRequest, 1024>& queue,
           MatchingEngine& engine);


    void start();
    void stop();
    void join();

private:
    void run();
    void pinThreadToCore(int core_id);

    RingBuffer<EMS::model::OrderRequest, 1024>& queue_;
    MatchingEngine& engine_;

    std::atomic<bool> running_{false};
    std::thread worker_;
};
