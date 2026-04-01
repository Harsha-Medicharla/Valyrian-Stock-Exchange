#pragma once

#include "Settlement/entities/Trade.h"
#include "Settlement/entities/Confirmation.h"
#include "Settlement/common/Pool.h"
#include "Settlement/funds/FundManager.h"
#include "Settlement/funds/ShareManager.h"
#include "Settlement/persistence/DBSyncWorker.h" 
#include "Settlement/confirmation/ConfirmationGenerator.h"
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <cstring>

namespace SettlementCore {

class Settlement {
public:
    Settlement(Pool<Trade>& q4, Pool<Confirmation>& q5) 
        : q4_pool(q4), q5_pool(q5), running(true) {
        // Start the background worker thread
        worker_thread = std::thread(&Settlement::processQueue, this);
    }

    ~Settlement() {
        running = false;
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
    // Note: The MatchingEngine now allocates from Q4 directly.
    // This method is kept for legacy/convenience but calls are being moved to direct Pool push.
    void settleTrade(uint64_t buyer_id, uint64_t seller_id, uint64_t symbol, int64_t price, int32_t qty) {
        // 1. Instant RAM Transfer (Atomic Swap)
        fund_manager.transferFunds(buyer_id, seller_id, price * qty);
        share_manager.transferShares(seller_id, buyer_id, symbol, qty);
    }

private:
    // --- BACKGROUND PROCESSING (SLOW PATH) ---
    void processQueue() {
        while (running) {
            Trade* trade = nullptr;
            if (!q4_pool.pop(trade)) {
                // Low-latency backoff if queue is empty
                std::this_thread::yield();
                continue;
            }

            if (!trade) continue;

            // 1. PERSISTENCE: Write to Postgres via the Sync Worker
            // Map string symbol back to numeric ID if possible, but for now we use 1.
            uint64_t symbol_id = 1; 
            try {
                symbol_id = std::stoull(trade->symbol);
            } catch (...) {}

            db_worker.persist(trade->buy_user_id, trade->sell_user_id, symbol_id, trade->exec_price, trade->exec_qty);

            // 2. CONFIRMATIONS: Generate and push to Q5 for both Buyer and Seller
            
            // Buyer Confirmation
            Confirmation* buy_conf = q5_pool.allocate();
            if (buy_conf) {
                buy_conf->trade_id = trade->trade_id;
                buy_conf->user_id = trade->buy_user_id;
                buy_conf->order_id = trade->buy_order_id;
                strncpy(buy_conf->symbol, trade->symbol, 8);
                buy_conf->exec_price = trade->exec_price;
                buy_conf->exec_qty = trade->exec_qty;
                buy_conf->remaining_qty = 0; // Simplified for now
                buy_conf->fund_delta = -(trade->exec_price * trade->exec_qty);
                buy_conf->share_delta = trade->exec_qty;
                buy_conf->side = 0; // BUY
                buy_conf->status = 1; // FILLED
                q5_pool.push(buy_conf);
            }

            // Seller Confirmation
            Confirmation* sell_conf = q5_pool.allocate();
            if (sell_conf) {
                sell_conf->trade_id = trade->trade_id;
                sell_conf->user_id = trade->sell_user_id;
                sell_conf->order_id = trade->sell_order_id;
                strncpy(sell_conf->symbol, trade->symbol, 8);
                sell_conf->exec_price = trade->exec_price;
                sell_conf->exec_qty = trade->exec_qty;
                sell_conf->remaining_qty = 0; // Simplified for now
                sell_conf->fund_delta = (trade->exec_price * trade->exec_qty);
                sell_conf->share_delta = -trade->exec_qty;
                sell_conf->side = 1; // SELL
                sell_conf->status = 1; // FILLED
                q5_pool.push(sell_conf);
            }
            
            // 3. DEALLOCATE Trade back to Q4 pool
            q4_pool.deallocate(trade);
        }
    }

    // Core Managers (Hot Path)
    FundManager fund_manager; 
    ShareManager share_manager;

    // External Connectors (Slow Path)
    DBSyncWorker db_worker; 
    
    // Pools
    Pool<Trade>& q4_pool;
    Pool<Confirmation>& q5_pool;

    // Threading
    std::atomic<bool> running;
    std::thread worker_thread;
};

} // namespace SettlementCore