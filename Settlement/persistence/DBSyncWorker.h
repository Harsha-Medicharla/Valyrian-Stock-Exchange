#pragma once
#include "Settlement/persistence/PostgresWriter.h"
#include <cstdint>

namespace SettlementCore {

class DBSyncWorker {
public:
    DBSyncWorker() = default;

    // The primary entry point for the background thread
    void persist(uint64_t buyer, uint64_t seller, uint64_t symbol, int64_t price, int32_t qty);

private:
    // This holds the actual database connection logic
    PostgresWriter pgWriter;
};

} // namespace SettlementCore