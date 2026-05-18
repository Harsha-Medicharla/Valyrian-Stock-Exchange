#pragma once
#include "../config/EMSConfig.h"
#include "../../db/SymbolCache.h"
#include "../types/RawOrder.h"
#include "../types/Common.h"
#include "RateLimiter.h"
#include "MarketState.h"
#include "BalanceCache.h"

class ValidationPipeline
{
private:
    bool checkBasicValidity(const RawOrder *o) const noexcept;
    bool checkFatFingerNotional(const RawOrder *o) const noexcept;
    bool checkMarketState(const RawOrder *o) const noexcept;
    bool checkRateLimit(const RawOrder *o) noexcept;
    bool checkTickSize(const RawOrder *o) const noexcept;
    bool checkLotSize(const RawOrder *o) const noexcept;
    bool checkBalance(const RawOrder *o) noexcept;

    RateLimiter &rateLimiter_;
    MarketState &marketState_;
    BalanceCache &balanceCache_;
    const SymbolCache &symbolCache_;

public:
    ValidationPipeline(RateLimiter &rl, MarketState &ms, BalanceCache &bc, const SymbolCache &sc);

    Decision process(const RawOrder *order, RejectReason &reason);
};

inline ValidationPipeline::ValidationPipeline(RateLimiter &rl, MarketState &ms, BalanceCache &bc,
                                              const SymbolCache &sc)
    : rateLimiter_(rl),
      marketState_(ms),
      balanceCache_(bc),
      symbolCache_(sc)
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

    if (!checkBalance(o))
    {
        r = RejectReason::INSUFFICIENT_FUNDS;
        return Decision::REJECT;
    }

    return Decision::ACCEPT;
}

inline bool ValidationPipeline::checkBasicValidity(const RawOrder *o) const noexcept
{
    if (o->cancel_flag != 0 || o->modify_flag != 0) return true;

    if (o->symbol_id >= marketState_.symbolCount())
        return false;

    if (o->type == static_cast<uint8_t>(OrderType::LIMIT)) {
        if (o->price < EMSConfig::MIN_PRICE || o->price > EMSConfig::MAX_PRICE)
            return false;
    } else if (o->type == static_cast<uint8_t>(OrderType::MARKET)) {
        if (o->price != 0)
            return false;
    } else {
        return false;
    }

    if (o->qty < EMSConfig::MIN_QTY || o->qty > EMSConfig::MAX_QTY)
        return false;

    if (o->side > 1)
        return false;

    return true;
}

inline bool ValidationPipeline::checkFatFingerNotional(const RawOrder *o) const noexcept
{
    if (o->cancel_flag != 0 || o->modify_flag != 0) return true;
    if (o->type == static_cast<uint8_t>(OrderType::MARKET)) return true;
    
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
    if (o->cancel_flag != 0 || o->modify_flag != 0) return true;
    if (o->type == static_cast<uint8_t>(OrderType::MARKET)) return true;
    
    return symbolCache_.isValidTick(o->symbol_id, o->price);
}

inline bool ValidationPipeline::checkLotSize(const RawOrder *o) const noexcept
{
    return symbolCache_.isValidLot(o->symbol_id, o->qty);
}

inline bool ValidationPipeline::checkBalance(const RawOrder *o) noexcept
{
    if (o->cancel_flag != 0 || o->modify_flag != 0) return true;

    if (o->side == 0)
    {
        const int64_t amount = o->price * static_cast<int64_t>(o->qty);
        return balanceCache_.tryBlockFunds(o->user_id, amount);
    }
    if (o->side == 1)
    {
        return balanceCache_.tryBlockHoldings(o->user_id, o->symbol_id, static_cast<int32_t>(o->qty));
    }
    return false;
}
