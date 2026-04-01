#include "Settlement/core/Settlement.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    SettlementCore::Settlement settlement;

    std::cout << "--- Starting Smoke Test ---" << std::endl;

    // 1. Setup: Give User 1 some cash and User 2 some Apple stock
    settlement.adminDeposit(1, 1000000);          // $10,000.00
    settlement.adminDepositShares(2, 101, 500);   // 500 shares of AAPL (ID: 101)

    // 2. Execute a Trade: User 1 buys 100 shares from User 2 at $150.00
    std::cout << "Settling trade..." << std::endl;
    settlement.settleTrade(1, 2, 101, 15000, 100);

    // 3. WAIT: Because the DB write is ASYNC, we must wait a second 
    // for the background thread to finish the Postgres INSERT.
    std::cout << "Waiting for background persistence..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "--- Smoke Test Complete ---" << std::endl;
    return 0;
}