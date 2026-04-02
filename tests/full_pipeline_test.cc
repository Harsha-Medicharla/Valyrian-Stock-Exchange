#include <iostream>
#include <thread>
#include <chrono>

#include "Settlement/core/Settlement.h"
#include "EMS/queue/RingBuffer.h"

using namespace SettlementCore;

int main() {
    std::cout << "--- FULL PIPELINE TEST START ---\n";

    const size_t QSIZE = 1024; // MUST match Settlement

    RingBuffer<Trade, QSIZE> q4;
    RingBuffer<Confirmation, QSIZE> q5;

    Settlement settlement(&q4, &q5);

    // Start settlement thread
    std::thread worker([&]() {
        settlement.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Setup balances
    settlement.adminDeposit(1, 1'000'000);
    settlement.adminDepositShares(2, 101, 500);

    // Simulate Matching Engine output
    Trade t{};
    t.trade_id = 1;
    t.buyer_id = 1;
    t.seller_id = 2;
    t.symbol = 101;
    t.price = 15000;
    t.qty = 100;

    t.checksum = t.trade_id ^ t.buyer_id ^ t.seller_id ^
                 t.symbol ^ t.price ^ t.qty;

    std::cout << "Pushing trade to Q4...\n";

    while (!q4.push(t)) {
        std::this_thread::yield();
    }

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Read confirmations
    Confirmation c;
    int count = 0;

    while (q5.pop(c)) {
        std::cout << "[CONFIRM] user=" << c.user_id
                  << " qty=" << c.exec_qty
                  << " fundΔ=" << c.fund_delta
                  << " shareΔ=" << c.share_delta
                  << "\n";
        count++;
    }

    std::cout << "Total confirmations: " << count << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "--- TEST COMPLETE ---\n";

    worker.detach();
    return 0;
}