// persistence/DBSyncWorker.h
#pragma once
#include "../entities/Trade.h"
#include "PostgresWriter.h"
#include <vector>
#include <mutex>

class DBSyncWorker {
public:
    void enqueueForPersistence(const Trade* trade);
    void flushToDatabase(); // Run by background thread

private:
    std::vector<Trade> pending_trades;
    std::mutex queue_mutex;
    PostgresWriter pgWriter;
};