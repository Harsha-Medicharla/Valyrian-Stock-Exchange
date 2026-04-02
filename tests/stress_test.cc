#include "Settlement/core/Settlement.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>


int main() {
    SettlementCore::Settlement settlement;
    const int MATCH_COUNT = 5000; // Total of 10,000 DB rows

    std::cout << "--- Starting Stress Test: 5,000 Matches ---" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < MATCH_COUNT; ++i) {
        // Hammer the engine with rapid settlements
        settlement.settleTrade(1001, 2002, 500, 12000 + i, 10);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "HOT PATH (Memory) finished in: " << diff.count() << " seconds" << std::endl;
    std::cout << "Matches per second: " << (MATCH_COUNT / diff.count()) << std::endl;
    
    std::cout << "Draining Background Queue (Slow Path)..." << std::endl;
    
    // Give Postgres a few seconds to catch up
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "--- Stress Test Complete ---" << std::endl;
    return 0;
}