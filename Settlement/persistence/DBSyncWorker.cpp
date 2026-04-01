#include "Settlement/persistence/DBSyncWorker.h"
#include <iostream>

namespace SettlementCore {

void DBSyncWorker::persist(uint64_t buyer, uint64_t seller, uint64_t symbol, int64_t price, int32_t qty) {
    try {
        // Log the attempt (optional, good for debugging server logs)
        // std::cout << "[DBSyncWorker] Persisting trade for symbol: " << symbol << std::endl;

        // Call the underlying SQL writer
        pgWriter.writeTrade(buyer, seller, symbol, price, qty);
        
    } catch (const std::exception& e) {
        // CRITICAL: On a server, we log errors to stderr so they show up in system logs
        std::cerr << "[DBSyncWorker] DATABASE ERROR: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[DBSyncWorker] Unknown error occurred during DB sync." << std::endl;
    }
}

} // namespace SettlementCore