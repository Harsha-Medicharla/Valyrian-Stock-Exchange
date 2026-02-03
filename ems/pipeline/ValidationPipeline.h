#pragma once
#include "../config/EMSConfig.h"
#include "../types/RawOrder.h"
#include "../types/Common.h"
#include "RateLimiter.h"
#include "MarketState.h"

class ValidationPipeline
{
private:
    bool checkBasicValidity(const RawOrder *o) const noexcept;
    bool checkFatFingerNotional(const RawOrder *o) const noexcept;
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
    if (!o)
    {
        r = RejectReason::INVALID_RANGE;
        return Decision::REJECT;
    }

    if (!checkBasicValidity(o))
    {
        r = RejectReason::INVALID_RANGE;
        return Decision::REJECT;
    }

    if (!checkFatFingerNotional(o))
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

inline bool ValidationPipeline::checkBasicValidity(const RawOrder *o) const noexcept
{
    if (!o)
        return false;

    if (o->symbol_id >= marketState_.symbolCount())
        return false;

    if (o->price < EMSConfig::MIN_PRICE || o->price > EMSConfig::MAX_PRICE)
        return false;
    if (o->qty < EMSConfig::MIN_QTY || o->qty > EMSConfig::MAX_QTY)
        return false;

    if (o->side > 1)
        return false;
    if (o->type > 1)
        return false;

    return true;
}

inline bool ValidationPipeline::checkFatFingerNotional(const RawOrder *o) const noexcept
{
    if (!o)
        return false;

    const int64_t notional = o->price * static_cast<int64_t>(o->qty);
    return notional <= EMSConfig::FAT_FINGER_LIMIT;
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
    return (o->price % EMSConfig::TICK_SIZE) == 0;
}

inline bool ValidationPipeline::checkLotSize(const RawOrder *o) const noexcept
{
    return (o->qty % EMSConfig::LOT_SIZE) == 0;
}
