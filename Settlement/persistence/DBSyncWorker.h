#pragma once

#include "Settlement/entities/Trade.h"
#include "Settlement/persistence/PostgresWriter.h"

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace SettlementCore {

class DBSyncWorker {
public:
    DBSyncWorker();
    ~DBSyncWorker();

    void start();
    void stop();

    void enqueue(const Trade& t);

private:
    void run();

    std::queue<Trade> queue;
    std::mutex mtx;
    std::condition_variable cv;

    std::thread worker;
    std::atomic<bool> running;

    PostgresWriter pgWriter;
};

}