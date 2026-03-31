#include "Settlement.h"
#include "../common/Pool.h"

void Settlement::processQueue() {
    Trade* trade = nullptr;
    
    // 1. Read Trade from Q4
    while (q4_trade_pool->pop(trade)) {
        
        // 2. Validate trade integrity
        if (!validator.isValid(trade)) {
            trade = validator.recoverFromWAL(trade->trade_id);
            if (!trade) continue; 
        }

        uint64_t total_trade_value = trade->exec_price * trade->exec_qty;

        // 3 & 4. Process Buyer
        fundManager.processBuyer(trade->buy_user_id, total_trade_value);

        // 5 & 6. Process Seller Shares
        shareManager.transferShares(trade->sell_user_id, trade->buy_user_id, 
                                    trade->symbol, trade->exec_qty);

        // 7. Process Seller Cash
        fundManager.processSeller(trade->sell_user_id, total_trade_value);

        // 8. Update Symbol Market Data
        symbolUpdater.updateMarketData(trade->symbol, trade->exec_price, trade->exec_qty);

        // 9. Handle partial fill leftovers
        int32_t buy_remaining = trade->buy_total_qty - trade->exec_qty;
        int32_t sell_remaining = trade->sell_total_qty - trade->exec_qty;
        partialFillHandler.process(trade, buy_remaining, sell_remaining);

        // 10. Generate confirmations + Push Q5
        confGenerator.generateAndPush(trade, buy_remaining, sell_remaining, q5_conf_pool);

        // 12. Async persist to DB
        dbSyncWorker.enqueueForPersistence(trade);

        // Final Step: Deallocate
        q4_trade_pool->deallocate(trade);
    }
}