#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
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
    std::vector<EventSPSC<DBEvent>> &ingressDbQueues_;
    std::vector<EventSPSC<DBEvent>> &engineDbQueues_;
    uint32_t numSymbols_{0};
    BalanceCache &balanceCache_;
    std::unique_ptr<IDBWriterBackend> ownedWriter_;
    IDBWriterBackend *writer_{nullptr};
    BatchBuffer batch_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    void run();
    void flush();

public:
    DBWriter(std::vector<EventSPSC<DBEvent>> &ingressDbQueues,
             std::vector<EventSPSC<DBEvent>> &engineDbQueues,
             uint32_t numSymbols,
             BalanceCache &balanceCache,
             const std::string &pgConnString)
        : ingressDbQueues_(ingressDbQueues),
          engineDbQueues_(engineDbQueues),
          numSymbols_(numSymbols),
          balanceCache_(balanceCache),
          ownedWriter_(std::make_unique<PGWriter>(pgConnString)),
          writer_(ownedWriter_.get())
    {
    }

    DBWriter(std::vector<EventSPSC<DBEvent>> &ingressDbQueues,
             std::vector<EventSPSC<DBEvent>> &engineDbQueues,
             uint32_t numSymbols,
             BalanceCache &balanceCache,
             IDBWriterBackend &writer)
        : ingressDbQueues_(ingressDbQueues),
          engineDbQueues_(engineDbQueues),
          numSymbols_(numSymbols),
          balanceCache_(balanceCache),
          writer_(&writer)
    {
    }

    void start();
    void stop() noexcept;
    void join();
};
