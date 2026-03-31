#include "TradeValidator.h"
#include <iostream>

bool TradeValidator::isValid(const Trade* trade) {
    // Detect duplicate processing
    if (processed_trades.find(trade->trade_id) != processed_trades.end()) {
        return false; 
    }
    
    // Verify Checksum
    uint32_t calculated_checksum = Checksum::calculate(trade);
    if (calculated_checksum != trade->checksum) {
        return false; // Corrupted in Q4
    }
    
    processed_trades.insert(trade->trade_id);
    return true;
}

Trade* TradeValidator::recoverFromWAL(uint64_t trade_id) {
    std::cerr << "[CRITICAL] Trade " << trade_id << " corrupted in memory! Reading WAL...\n";
    // Once WAL is implemented, read the file and return a clean Trade*.
    // For now, return nullptr to drop the corrupted trade safely without crashing.
    return nullptr;
}