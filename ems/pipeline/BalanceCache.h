#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include "../config/EMSConfig.h"
#include "../../shared/types/CoreTypes.h"

// In-memory balances (per user) and holdings (per user × symbol).
// Hot path: IngressWorker CAS loops; DBWriter calls settle/unblock (single writer).

struct alignas(64) BalanceEntry
{
    std::atomic<int64_t> available{0};
    std::atomic<int64_t> blocked{0};
    uint8_t _pad[48]{};
};

struct alignas(64) HoldingsGroup
{
    struct Slot
    {
        std::atomic<int32_t> available_qty{0};
        std::atomic<int32_t> blocked_qty{0};
    } slots[8];
};

class BalanceCache
{
private:
    static constexpr std::size_t kMaxUsers = EMSConfig::MAX_USERS;

    std::unique_ptr<BalanceEntry[]> balances_;
    std::unique_ptr<HoldingsGroup[]> holdingsGroups_;
    std::size_t numSymbols_{0};
    std::size_t numHoldingsSlots_{0};
    std::size_t numHoldingsGroups_{0};

    [[nodiscard]] inline std::size_t holdingsFlatIndex(uint32_t userId, uint32_t symbolId) const noexcept
    {
        const std::size_t u = static_cast<std::size_t>(userId) % kMaxUsers;
        return u * numSymbols_ + static_cast<std::size_t>(symbolId);
    }

    inline HoldingsGroup::Slot &holdingsSlot(uint32_t userId, uint32_t symbolId) noexcept
    {
        const std::size_t flat = holdingsFlatIndex(userId, symbolId);
        const std::size_t group = flat / 8;
        const std::size_t idx = flat % 8;
        return holdingsGroups_[group].slots[idx];
    }

    inline const HoldingsGroup::Slot &holdingsSlot(uint32_t userId, uint32_t symbolId) const noexcept
    {
        const std::size_t flat = holdingsFlatIndex(userId, symbolId);
        const std::size_t group = flat / 8;
        const std::size_t idx = flat % 8;
        return holdingsGroups_[group].slots[idx];
    }

public:
    explicit BalanceCache(std::size_t numSymbols) noexcept
        : balances_(std::make_unique<BalanceEntry[]>(kMaxUsers)),
          numSymbols_(numSymbols),
          numHoldingsSlots_(kMaxUsers * numSymbols_),
          numHoldingsGroups_((numHoldingsSlots_ + 7) / 8)
    {
        holdingsGroups_ = std::make_unique<HoldingsGroup[]>(numHoldingsGroups_);
    }

    BalanceCache(const BalanceCache &) = delete;
    BalanceCache &operator=(const BalanceCache &) = delete;

    [[nodiscard]] bool tryBlockFunds(uint32_t userId, int64_t amount) noexcept
    {
        if (amount <= 0)
            return true;
        BalanceEntry &be = balances_[static_cast<std::size_t>(userId) % kMaxUsers];
        int64_t av = be.available.load(std::memory_order_relaxed);
        while (true)
        {
            if (av < amount)
                return false;
            if (be.available.compare_exchange_weak(
                    av, av - amount,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
            {
                be.blocked.fetch_add(amount, std::memory_order_release);
                return true;
            }
        }
    }

    [[nodiscard]] bool tryBlockHoldings(uint32_t userId, uint32_t symbolId, int32_t qty) noexcept
    {
        if (qty <= 0)
            return true;
        HoldingsGroup::Slot &sl = holdingsSlot(userId, symbolId);
        int32_t av = sl.available_qty.load(std::memory_order_relaxed);
        while (true)
        {
            if (av < qty)
                return false;
            if (sl.available_qty.compare_exchange_weak(
                    av, av - qty,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
            {
                sl.blocked_qty.fetch_add(qty, std::memory_order_release);
                return true;
            }
        }
    }

    void settleTrade(uint32_t buyUserId, uint32_t sellUserId,
                     uint32_t symbolId, Price price, Qty qty) noexcept
    {
        if (qty <= 0)
            return;
        const int64_t notional = price * qty;

        BalanceEntry &buyer = balances_[static_cast<std::size_t>(buyUserId) % kMaxUsers];
        buyer.blocked.fetch_sub(notional, std::memory_order_release);

        HoldingsGroup::Slot &buyH = holdingsSlot(buyUserId, symbolId);
        buyH.available_qty.fetch_add(static_cast<int32_t>(qty), std::memory_order_release);

        HoldingsGroup::Slot &sellH = holdingsSlot(sellUserId, symbolId);
        sellH.blocked_qty.fetch_sub(static_cast<int32_t>(qty), std::memory_order_release);

        BalanceEntry &seller = balances_[static_cast<std::size_t>(sellUserId) % kMaxUsers];
        seller.available.fetch_add(notional, std::memory_order_release);
    }

    void unblockFunds(uint32_t userId, int64_t amount) noexcept
    {
        if (amount <= 0)
            return;
        BalanceEntry &be = balances_[static_cast<std::size_t>(userId) % kMaxUsers];
        be.blocked.fetch_sub(amount, std::memory_order_release);
        be.available.fetch_add(amount, std::memory_order_release);
    }

    void unblockHoldings(uint32_t userId, uint32_t symbolId, int32_t qty) noexcept
    {
        if (qty <= 0)
            return;
        HoldingsGroup::Slot &sl = holdingsSlot(userId, symbolId);
        sl.blocked_qty.fetch_sub(qty, std::memory_order_release);
        sl.available_qty.fetch_add(qty, std::memory_order_release);
    }

    void addAvailable(uint32_t userId, int64_t amount) noexcept
    {
        if (amount <= 0)
            return;
        BalanceEntry &be = balances_[static_cast<std::size_t>(userId) % kMaxUsers];
        be.available.fetch_add(amount, std::memory_order_release);
    }

    void setBalance(uint32_t userId, int64_t available, int64_t blocked) noexcept
    {
        BalanceEntry &be = balances_[static_cast<std::size_t>(userId) % kMaxUsers];
        be.available.store(available, std::memory_order_release);
        be.blocked.store(blocked, std::memory_order_release);
    }

    void setHoldings(uint32_t userId, uint32_t symbolId,
                     int32_t availableQty, int32_t blockedQty) noexcept
    {
        HoldingsGroup::Slot &sl = holdingsSlot(userId, symbolId);
        sl.available_qty.store(availableQty, std::memory_order_release);
        sl.blocked_qty.store(blockedQty, std::memory_order_release);
    }

    [[nodiscard]] int64_t availableBalance(uint32_t userId) const noexcept
    {
        return balances_[static_cast<std::size_t>(userId) % kMaxUsers].available.load(std::memory_order_acquire);
    }

    [[nodiscard]] int64_t blockedBalance(uint32_t userId) const noexcept
    {
        return balances_[static_cast<std::size_t>(userId) % kMaxUsers].blocked.load(std::memory_order_acquire);
    }

    [[nodiscard]] int32_t availableHoldings(uint32_t userId, uint32_t symbolId) const noexcept
    {
        return holdingsSlot(userId, symbolId).available_qty.load(std::memory_order_acquire);
    }

    [[nodiscard]] int32_t blockedHoldings(uint32_t userId, uint32_t symbolId) const noexcept
    {
        return holdingsSlot(userId, symbolId).blocked_qty.load(std::memory_order_acquire);
    }
};
