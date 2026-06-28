#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "BatchBuffer.h"
#include "PGWriter.h"
#include "ems/pipeline/BalanceCache.h"
#include "shared/queues/EventSPSC.h"
#include "shared/types/Events.h"

class DBWriter
{
private:
    std::vector<EventSPSC<DBEvent>> &engineDbQueues_;
    std::vector<uint32_t> assignedSymbols_;
    BalanceCache &balanceCache_;
    std::unique_ptr<IDBWriterBackend> ownedWriter_;
    IDBWriterBackend *writer_{nullptr};
    BatchBuffer batch_;
    std::atomic<bool> running_{false};
    std::thread thread_;

    void run();
    void flush();

public:
    DBWriter(std::vector<EventSPSC<DBEvent>> &engineDbQueues,
             std::vector<uint32_t> assignedSymbols,
             BalanceCache &balanceCache,
             const std::string &pgConnString)
        : engineDbQueues_(engineDbQueues),
          assignedSymbols_(std::move(assignedSymbols)),
          balanceCache_(balanceCache),
          ownedWriter_(std::make_unique<PGWriter>(pgConnString)),
          writer_(ownedWriter_.get())
    {
    }

    DBWriter(std::vector<EventSPSC<DBEvent>> &engineDbQueues,
             std::vector<uint32_t> assignedSymbols,
             BalanceCache &balanceCache,
             IDBWriterBackend &writer)
        : engineDbQueues_(engineDbQueues),
          assignedSymbols_(std::move(assignedSymbols)),
          balanceCache_(balanceCache),
          writer_(&writer)
    {
    }

    void start();
    void stop() noexcept;
    void join();
};
