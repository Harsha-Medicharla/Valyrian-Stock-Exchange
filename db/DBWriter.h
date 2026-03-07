#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "BatchBuffer.h"
#include "PGWriter.h"
#include "ems/pipeline/BalanceCache.h"
#include "shared/queues/EventSPSC.h"
#include "shared/types/Events.h"

class DBWriter
{
private:
    std::vector<EventSPSC<DBEvent>> &dbQueues_;
    uint32_t numSymbols_{0};
    BalanceCache &balanceCache_;
    PGWriter pgWriter_;
    BatchBuffer batch_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    void run();
    void flush();

public:
    DBWriter(std::vector<EventSPSC<DBEvent>> &dbQueues, uint32_t numSymbols, BalanceCache &balanceCache,
             const std::string &pgConnString)
        : dbQueues_(dbQueues),
          numSymbols_(numSymbols),
          balanceCache_(balanceCache),
          pgWriter_(pgConnString)
    {
    }

    void start();
    void stop();
    void join();
};
