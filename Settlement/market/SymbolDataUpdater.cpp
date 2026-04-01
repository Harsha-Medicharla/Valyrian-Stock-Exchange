#include "SymbolDataUpdater.h"

void SymbolDataUpdater::updateMarketData(const char* symbol, int64_t exec_price, int32_t exec_qty) {
    auto& data = market_data[symbol];
    data.ltp = exec_price;
    data.volume += exec_qty;
    
    if (exec_price > data.high) data.high = exec_price;
    if (exec_price < data.low || data.low == 0) data.low = exec_price;
}