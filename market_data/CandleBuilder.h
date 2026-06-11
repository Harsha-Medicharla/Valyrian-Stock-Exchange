#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "shared/types/CoreTypes.h"

struct Candle
{
    uint64_t open_ns;
    Price open;
    Price high;
    Price low;
    Price close;
    Qty volume;
    uint32_t symbol_id;
    uint32_t window_seconds;
};

class CandleBuilder
{
private:
    std::vector<Candle> candles_;

public:
    explicit CandleBuilder(uint32_t numSymbols);

    [[nodiscard]] std::string onTrade(uint32_t symbolId, Price price, Qty qty, TimeStamp ts);
};
