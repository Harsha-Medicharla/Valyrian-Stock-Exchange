#pragma once

#include "../entities/Trade.h"
#include <fstream>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>

class WAL {
public:
    WAL(const std::string& filename);
    ~WAL();

    bool log(const Trade& t);
    void replay(std::function<void(const Trade&)> callback);

private:
    std::ofstream out;
    std::string filename;
    std::mutex mtx;

    std::thread fsync_thread;
    std::atomic<bool> running;

    uint32_t checksum(const Trade& t);

    void fsyncLoop();
};