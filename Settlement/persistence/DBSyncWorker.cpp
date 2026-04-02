#include "Settlement/persistence/DBSyncWorker.h"
#include <iostream>

namespace SettlementCore {

DBSyncWorker::DBSyncWorker() : running(false) {}

DBSyncWorker::~DBSyncWorker() {
    stop();
}

void DBSyncWorker::start() {
    running = true;
    worker = std::thread(&DBSyncWorker::run, this);
}

void DBSyncWorker::stop() {
    running = false;
    cv.notify_all();

    if (worker.joinable()) {
        worker.join();
    }
}

void DBSyncWorker::enqueue(const Trade& t) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(t);
    }
    cv.notify_one();
}

void DBSyncWorker::run() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [&]() {
            return !queue.empty() || !running;
        });

        while (!queue.empty()) {
            Trade t = queue.front();
            queue.pop();

            lock.unlock();

            try {
                pgWriter.writeTrade(
                    t.trade_id,
                    t.buyer_id,
                    t.seller_id,
                    t.symbol,
                    t.price,
                    t.qty
                );
            } catch (const std::exception& e) {
                std::cerr << "[DBSyncWorker] DB ERROR: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[DBSyncWorker] Unknown DB error\n";
            }

            lock.lock();
        }
    }
}

}