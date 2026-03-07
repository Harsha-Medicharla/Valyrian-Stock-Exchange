#include "CandleBuilder.h"

#include <sstream>

CandleBuilder::CandleBuilder(uint32_t numSymbols)
{
    candles_.resize(static_cast<std::size_t>(numSymbols) * 3u);
}

std::string CandleBuilder::onTrade(uint32_t symbolId, Price price, Qty qty, TimeStamp ts)
{
    (void)symbolId;
    (void)price;
    (void)qty;
    (void)ts;
    return {};
}
