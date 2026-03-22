#pragma once
#include <atomic>
#include <thread>
#include "../model/OrderRequest.h"
#include "../queue/RingBuffer.h"

class MatchingEngine; 

namespace EMS {
    class EMSCore; 

    class Dispatcher {
    public:
        Dispatcher(RingBuffer<model::OrderRequest, 1024>& queue, MatchingEngine& engine);
        void start();
        void stop();
        void join();
    private:
        void run();
        void pinThreadToCore(int core_id); 

        RingBuffer<model::OrderRequest, 1024>& queue_;
        MatchingEngine& engine_;
        std::atomic<bool> running_{false};
        std::thread worker_;
    };
}