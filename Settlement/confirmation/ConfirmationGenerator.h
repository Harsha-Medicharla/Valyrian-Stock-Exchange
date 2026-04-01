#pragma once
#include <iostream>
#include <cstdint>
#include <string>

namespace SettlementCore {

class ConfirmationGenerator {
public:
    // We make this static so Settlement.h can call it without an instance
    static void generate(uint64_t buyer, uint64_t seller, uint64_t symbol, int64_t price, int32_t qty) {
        std::cout << "[CONFIRMATION] Trade Executed | Symbol: " << symbol 
                  << " | Price: " << (price / 100.0) 
                  << " | Qty: " << qty 
                  << " | Buyer: " << buyer 
                  << " | Seller: " << seller << std::endl;
    }
};

} // namespace SettlementCore