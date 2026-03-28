#pragma once
#include "../types/RawOrder.h"
#include "../types/Common.h"
#include "RateLimiter.h"
#include "MarketState.h"


constexpr int64_t kMinPrice = 1;
constexpr int64_t kMaxPrice = 1'000'000'000;
constexpr uint32_t kMinQty = 1;
constexpr uint32_t kMaxQty = 1'000'000'000;
// Domain placeholders. Keep them const/constexpr so the compiler can fold.
constexpr int64_t kTickSize = 1;
constexpr uint32_t kLotSize = 1;


class ValidationPipeline
{
private:
    bool checkFatFinger(const RawOrder *o) const noexcept;
    bool checkMarketState(const RawOrder *o) const noexcept;
    bool checkRateLimit(const RawOrder *o) noexcept;
    bool checkTickSize(const RawOrder *o) const noexcept;
    bool checkLotSize(const RawOrder *o) const noexcept;

    RateLimiter &rateLimiter_;
    MarketState &marketState_;

public:
    ValidationPipeline(RateLimiter &rl, MarketState &ms);

    Decision process(const RawOrder *order, RejectReason &reason);

    void decay() noexcept;
};

inline ValidationPipeline::ValidationPipeline(RateLimiter &rl, MarketState &ms)
    : rateLimiter_(rl),
      marketState_(ms)
{
}

inline Decision ValidationPipeline::process(const RawOrder *o, RejectReason &r)
{
    // Fail-fast null guard (should never happen on hot path).
    if (!o)
    {
        r = RejectReason::FAT_FINGER;
        return Decision::REJECT;
    }

    if (!checkFatFinger(o))
    {
        r = RejectReason::FAT_FINGER;
        return Decision::REJECT;
    }

    if (!checkMarketState(o))
    {
        r = RejectReason::MARKET_CLOSED;
        return Decision::REJECT;
    }

    if (!checkRateLimit(o))
    {
        r = RejectReason::RATE_LIMIT;
        return Decision::REJECT;
    }

    if (!checkTickSize(o))
    {
        r = RejectReason::INVALID_TICK;
        return Decision::REJECT;
    }

    if (!checkLotSize(o))
    {
        r = RejectReason::INVALID_LOT;
        return Decision::REJECT;
    }

    return Decision::ACCEPT;
}

inline void ValidationPipeline::decay() noexcept
{
    rateLimiter_.decay();
}

inline bool ValidationPipeline::checkFatFinger(const RawOrder *o) const noexcept
{
    if (!o)
        return false;

    // Basic sanity checks; ensures deterministic rejection reasons.
    if (o->price < kMinPrice || o->price > kMaxPrice)
        return false;
    if (o->qty < kMinQty || o->qty > kMaxQty)
        return false;

    // Validate enum-like fields are in-range.
    if (o->side > 1)
        return false;
    if (o->type > 1)
        return false;

    return true;
}

inline bool ValidationPipeline::checkMarketState(const RawOrder *o) const noexcept
{
    return marketState_.isSymbolOpen(o->symbol_id);
}

inline bool ValidationPipeline::checkRateLimit(const RawOrder *o) noexcept
{
    return rateLimiter_.checkAndIncrement(o->user_id);
}

inline bool ValidationPipeline::checkTickSize(const RawOrder *o) const noexcept
{
    // Tick size check (placeholder: kTickSize defaults to 1).
    return (o->price % kTickSize) == 0;
}

inline bool ValidationPipeline::checkLotSize(const RawOrder *o) const noexcept
{
    // Lot size check (placeholder: kLotSize defaults to 1).
    return (o->qty % kLotSize) == 0;
}
