#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

#include "Settlement/common/Pool.h"
#include "Settlement/common/SettlementConfig.h"
#include "Settlement/core/Settlement.h"

// Global flag to shut down the test gracefully
std::atomic<bool> exchange_running{true};

// --- PRODUCER: Mock Matching Engine ---
void runMockMatchingEngine(Pool<Trade>* q4_pool) {
    uint64_t trade_counter = 1;
    
    std::cout << "[ME] Matching Engine started.\n";
    
    while (exchange_running) {
        // 1. Allocate a Trade object from the memory pool
        Trade* new_trade = q4_pool->allocate();
        
        if (new_trade) {
            // 2. Populate the trade data
            new_trade->trade_id = trade_counter++;
            new_trade->buy_user_id = 100;
            new_trade->sell_user_id = 200;
            std::strncpy(new_trade->symbol, "AAPL", 8);
            new_trade->exec_price = 15000; // 150.00 in paise
            new_trade->exec_qty = 10;
            new_trade->buy_total_qty = 10;
            new_trade->sell_total_qty = 10;
            
            // 3. Calculate checksum
            new_trade->checksum = Checksum::calculate(new_trade);
            
            // 4. Push to Q4
            while (!q4_pool->push(new_trade) && exchange_running) {
                std::this_thread::yield(); // Back off if queue is full
            }
            
            if (trade_counter % 1000 == 0) {
                std::cout << "[ME] Generated " << trade_counter << " trades...\n";
            }
        }
        
        // Simulate a tiny delay between matches
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    std::cout << "[ME] Matching Engine stopped.\n";
}

// --- CONSUMER: Settlement Pipeline ---
void runSettlement(Settlement* settlement_module) {
    std::cout << "[SETTLEMENT] Pipeline started.\n";
    
    while (exchange_running) {
        // Drain Q4, process, and push to Q5
        settlement_module->processQueue();
        std::this_thread::yield(); 
    }
    
    // One last drain to clear the queue after ME stops
    settlement_module->processQueue();
    std::cout << "[SETTLEMENT] Pipeline stopped.\n";
}

// --- MAIN TEST RUNNER ---
int main() {
    std::cout << "--- Starting Settlement Module Test ---\n";

    // 1. Setup config and sizes
    SettlementConfig config;
    config.q4_pool_size = 50000;
    config.q5_pool_size = 100000; 

    // 2. Instantiate Pools
    Pool<Trade> q4_trade_pool(config.q4_pool_size);
    Pool<Confirmation> q5_conf_pool(config.q5_pool_size);

    // 3. Instantiate Settlement System
    Settlement settlement(&q4_trade_pool, &q5_conf_pool);

    // 4. Launch Threads
    std::thread me_thread(runMockMatchingEngine, &q4_trade_pool);
    std::thread settlement_thread(runSettlement, &settlement);

    // 5. Let it run for 3 seconds, then shut down
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "--- Initiating Shutdown ---\n";
    exchange_running = false;

    // 6. Wait for threads to finish
    me_thread.join();
    settlement_thread.join();

    std::cout << "--- Test Complete ---\n";
    return 0;
}