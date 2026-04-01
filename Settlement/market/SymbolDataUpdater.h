// market/SymbolDataUpdater.h
#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>

struct MarketData {
    int64_t ltp = 0;
    int64_t high = 0;
    int64_t low = 0;
    int64_t volume = 0;
};

class SymbolDataUpdater {
public:
    void updateMarketData(const char* symbol, int64_t exec_price, int32_t exec_qty);

private:
    std::unordered_map<std::string, MarketData> market_data;
};