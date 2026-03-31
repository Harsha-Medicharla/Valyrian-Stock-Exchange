#include "DBSyncWorker.h"
#include <iostream>

void DBSyncWorker::enqueueForPersistence(const Trade* trade) {
    bool should_flush = false;
    
    // Create a local scope for the lock so it unlocks BEFORE calling flush
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_trades.push_back(*trade); 
        
        if (pending_trades.size() >= 1000) {
            should_flush = true;
        }
    } // <-- Mutex automatically unlocks right here!

    // Now it's safe to call flush, which has its own lock
    if (should_flush) {
        flushToDatabase();
    }
}

void DBSyncWorker::flushToDatabase() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (pending_trades.empty()) return;

    // Send the batch to the Postgres writer
    pgWriter.executeBatch(pending_trades);
    
    // Clear the queue for the next batch
    pending_trades.clear();
}