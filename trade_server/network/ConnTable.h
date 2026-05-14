#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <memory>

#include "ems/config/EMSConfig.h"

struct alignas(64) ConnState
{
    uint64_t user_id;
    uint64_t last_client_seq;
    bool     is_active;
    uint8_t  _pad[47];
};

static_assert(sizeof(ConnState) == 64, "ConnState layout");

class ConnTable
{
private:
    static constexpr std::uint32_t kMax = 65536;

    alignas(64) std::array<ConnState, kMax> slots_{};
    alignas(64) std::array<std::uint32_t, kMax> nextFree_{};
    std::atomic<std::uint32_t> freeTop_{0};
    std::atomic<std::uint32_t> freeSize_{0};
    std::unique_ptr<std::atomic<std::uint32_t>[]> activeByUser_;

public:
    ConnTable() noexcept
        : activeByUser_(std::make_unique<std::atomic<std::uint32_t>[]>(EMSConfig::MAX_USERS))
    {
        for (auto &s : slots_)
        {
            s.is_active = false;
            s.user_id = 0;
            s.last_client_seq = 0;
        }
        for (std::size_t i = 0; i < EMSConfig::MAX_USERS; ++i)
            activeByUser_[i].store(0, std::memory_order_relaxed);
        for (std::uint32_t id = 1; id < kMax - 1; ++id)
            nextFree_[id] = id + 1;
        nextFree_[kMax - 1] = 0;
        freeTop_.store(1, std::memory_order_relaxed);
        freeSize_.store(kMax - 1, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint32_t assign(std::uint64_t userId) noexcept
    {
        std::uint32_t id = freeTop_.load(std::memory_order_acquire);
        while (id != 0)
        {
            const std::uint32_t next = nextFree_[id];
            if (freeTop_.compare_exchange_weak(
                    id, next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire))
            {
                freeSize_.fetch_sub(1, std::memory_order_relaxed);
                break;
            }
        }
        if (id == 0)
            return 0;

        ConnState &s = slots_[id];
        s.user_id = userId;
        s.last_client_seq = 0;
        s.is_active = true;
        activeByUser_[userId % EMSConfig::MAX_USERS].store(id, std::memory_order_release);
        return id;
    }

    void release(std::uint32_t connId) noexcept
    {
        if (connId == 0 || connId >= kMax)
            return;
        ConnState &state = slots_[connId];
        state.is_active = false;
        activeByUser_[state.user_id % EMSConfig::MAX_USERS].store(0, std::memory_order_release);

        std::uint32_t top = freeTop_.load(std::memory_order_acquire);
        do
        {
            nextFree_[connId] = top;
        } while (!freeTop_.compare_exchange_weak(
            top, connId,
            std::memory_order_acq_rel,
            std::memory_order_acquire));
        freeSize_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] ConnState &get(std::uint32_t connId) noexcept { return slots_[connId]; }

    [[nodiscard]] std::uint32_t findByUser(std::uint64_t userId) const noexcept
    {
        return activeByUser_[userId % EMSConfig::MAX_USERS].load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint32_t size() const noexcept
    {
        return freeSize_.load(std::memory_order_relaxed);
    }
};
