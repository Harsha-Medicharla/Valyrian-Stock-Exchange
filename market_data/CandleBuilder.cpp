#include "CandleBuilder.h"

#include <array>
#include <sstream>

CandleBuilder::CandleBuilder(uint32_t numSymbols)
{
    candles_.resize(static_cast<std::size_t>(numSymbols) * 3u);
}

std::string CandleBuilder::onTrade(uint32_t symbolId, Price price, Qty qty, TimeStamp ts)
{
    static constexpr std::array<uint32_t, 3> kWindows = {60u, 300u, 900u};
    static constexpr uint64_t kNsPerSecond = 1'000'000'000ULL;

    std::string closedJson;
    for (std::size_t windowIdx = 0; windowIdx < kWindows.size(); ++windowIdx)
    {
        Candle &candle = candles_[static_cast<std::size_t>(symbolId) * 3u + windowIdx];
        const uint32_t windowSeconds = kWindows[windowIdx];

        if (candle.open_ns == 0)
        {
            candle.open_ns = ts;
            candle.open = price;
            candle.high = price;
            candle.low = price;
            candle.close = price;
            candle.volume = qty;
            candle.symbol_id = symbolId;
            candle.window_seconds = windowSeconds;
            continue;
        }

        if (ts - candle.open_ns < static_cast<uint64_t>(windowSeconds) * kNsPerSecond)
        {
            if (price > candle.high)
                candle.high = price;
            if (price < candle.low)
                candle.low = price;
            candle.close = price;
            candle.volume += qty;
            continue;
        }

        if (closedJson.empty())
        {
            std::ostringstream os;
            os << "{\"type\":\"candle\",\"symbol_id\":" << candle.symbol_id
               << ",\"window\":" << candle.window_seconds
               << ",\"open\":" << candle.open
               << ",\"high\":" << candle.high
               << ",\"low\":" << candle.low
               << ",\"close\":" << candle.close
               << ",\"volume\":" << candle.volume
               << ",\"open_ns\":" << candle.open_ns << "}";
            closedJson = os.str();
        }

        candle.open_ns = ts;
        candle.open = price;
        candle.high = price;
        candle.low = price;
        candle.close = price;
        candle.volume = qty;
        candle.symbol_id = symbolId;
        candle.window_seconds = windowSeconds;
    }

    return closedJson;
}
