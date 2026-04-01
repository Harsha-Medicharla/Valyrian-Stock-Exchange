#pragma once

#include "Settlement/funds/FundManager.h"
#include "Settlement/funds/ShareManager.h"
#include "Settlement/persistence/PostgresWriter.h" 
#include <cstdint>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace SettlementCore {

// --- IMPORTANT: This MUST be defined ABOVE the Settlement class ---
struct TradeRecord {
    uint64_t buyer_id;
    uint64_t seller_id;
    uint64_t symbol;
    int64_t price;
    int32_t qty;
};

class Settlement {
public:
    Settlement() : running(true), pg_writer() {
        worker_thread = std::thread(&Settlement::processQueue, this);
    }

    ~Settlement() {
        running = false;
        cv.notify_all(); 
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    void adminDeposit(uint64_t user_id, int64_t amount) {
        fund_manager.deposit(user_id, amount);
    }
    
    void adminDepositShares(uint64_t user_id, uint64_t symbol, int32_t qty) {
        share_manager.deposit(user_id, symbol, qty);
    }

    bool reserveMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
        if (side == 1) return fund_manager.reserveFunds(user_id, price * qty);
        return share_manager.reserve(user_id, symbol, qty);
    }

    void releaseMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
        if (side == 1) fund_manager.releaseFunds(user_id, price * qty);
        else share_manager.release(user_id, symbol, qty);
    }

    void settleTrade(uint64_t buyer_id, uint64_t seller_id, uint64_t symbol, int64_t price, int32_t qty) {
        // 1. Hot Path Update
        fund_manager.transferFunds(buyer_id, seller_id, price * qty);
        share_manager.transferShares(seller_id, buyer_id, symbol, qty);

        // 2. Queue for Background Thread
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            q4_trades.push({buyer_id, seller_id, symbol, price, qty});
        }
        cv.notify_one(); 
    }

private:
    void processQueue() {
        while (running || !q4_trades.empty()) {
            TradeRecord trade; // Now the compiler knows what this is!
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this]() { return !q4_trades.empty() || !running; });
                
                if (!running && q4_trades.empty()) return;

                trade = q4_trades.front();
                q4_trades.pop();
            }

            // Database write (Slow Path)
            try {
                // Adjust this method name to match your PostgresWriter.h exactly
                // pg_writer.writeTrade(trade.buyer_id, trade.seller_id, trade.symbol, trade.price, trade.qty);
            } catch (...) {}
        }
    }

    FundManager fund_manager; 
    ShareManager share_manager;
    PostgresWriter pg_writer; 
    
    std::atomic<bool> running;
    std::thread worker_thread;
    std::queue<TradeRecord> q4_trades; 
    std::mutex queue_mutex;
    std::condition_variable cv;
};

} // namespace SettlementCore