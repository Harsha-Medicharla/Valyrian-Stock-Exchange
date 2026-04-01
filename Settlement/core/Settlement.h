#pragma once

#include "Settlement/funds/FundManager.h"
#include "Settlement/funds/ShareManager.h"
#include "Settlement/persistence/DBSyncWorker.h" 
#include "Settlement/confirmation/ConfirmationGenerator.h"
#include <cstdint>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace SettlementCore {

struct TradeRecord {
    uint64_t buyer_id;
    uint64_t seller_id;
    uint64_t symbol;
    int64_t price;
    int32_t qty;
};

class Settlement {
public:
    Settlement() : running(true) {
        // Start the background worker thread
        worker_thread = std::thread(&Settlement::processQueue, this);
    }

    ~Settlement() {
        // Signal thread to stop
        running = false;
        cv.notify_all(); 
        
        // Wait for thread to finish processing remaining queue
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    // --- ADMIN / TEST INTERFACE ---
    void adminDeposit(uint64_t user_id, int64_t amount) {
        fund_manager.deposit(user_id, amount);
    }
    
    void adminDepositShares(uint64_t user_id, uint64_t symbol, int32_t qty) {
        share_manager.deposit(user_id, symbol, qty);
    }

    // --- PRE-TRADE RISK (HOT PATH) ---
    bool reserveMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
        if (side == 1) return fund_manager.reserveFunds(user_id, price * qty);
        return share_manager.reserve(user_id, symbol, qty);
    }

    void releaseMargin(uint64_t user_id, uint64_t symbol, uint8_t side, int64_t price, int32_t qty) {
        if (side == 1) fund_manager.releaseFunds(user_id, price * qty);
        else share_manager.release(user_id, symbol, qty);
    }

    // --- TRADE SETTLEMENT (HOT PATH) ---
    void settleTrade(uint64_t buyer_id, uint64_t seller_id, uint64_t symbol, int64_t price, int32_t qty) {
        // 1. Instant RAM Transfer (Atomic Swap)
        fund_manager.transferFunds(buyer_id, seller_id, price * qty);
        share_manager.transferShares(seller_id, buyer_id, symbol, qty);

        // 2. Offload to Slow Path (Persistence & Notifications)
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            q4_trades.push({buyer_id, seller_id, symbol, price, qty});
        }
        cv.notify_one(); 
    }

private:
    // --- BACKGROUND PROCESSING (SLOW PATH) ---
    void processQueue() {
        while (running || !q4_trades.empty()) {
            TradeRecord trade;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this]() { return !q4_trades.empty() || !running; });
                
                // Exit condition: not running and no more trades to save
                if (!running && q4_trades.empty()) return;

                trade = q4_trades.front();
                q4_trades.pop();
            }

            // 1. PERSISTENCE: Write to Postgres via the Sync Worker
            // This is decoupled so if the DB hangs, the matching engine doesn't
            db_worker.persist(trade.buyer_id, trade.seller_id, trade.symbol, trade.price, trade.qty);

            // 2. CONFIRMATIONS: Notify the users/external systems
            ConfirmationGenerator::generate(trade.buyer_id, trade.seller_id, trade.symbol, trade.price, trade.qty);
        }
    }

    // Core Managers (Hot Path)
    FundManager fund_manager; 
    ShareManager share_manager;

    // External Connectors (Slow Path)
    DBSyncWorker db_worker; 
    
    // Threading
    std::atomic<bool> running;
    std::thread worker_thread;
    std::queue<TradeRecord> q4_trades; 
    std::mutex queue_mutex;
    std::condition_variable cv;
};

} // namespace SettlementCore