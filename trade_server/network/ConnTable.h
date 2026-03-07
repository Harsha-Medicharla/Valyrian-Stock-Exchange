#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

struct alignas(64) ConnState
{
    uint64_t user_id;
    uint64_t last_client_seq;
    bool     is_active;
    uint8_t  _pad[47];
};

static_assert(sizeof(ConnState) == 64, "ConnState layout");

// Minimal connection table: monotonic ids (production would recycle via a lock-free stack).
class ConnTable
{
private:
    static constexpr std::uint32_t kMax = 65536;

    alignas(64) std::array<ConnState, kMax> slots_{};
    std::atomic<std::uint32_t> nextId_{1};
    mutable std::shared_mutex activeByUserMutex_;
    std::unordered_map<std::uint64_t, std::uint32_t> activeByUser_;

public:
    ConnTable() noexcept
    {
        for (auto &s : slots_)
        {
            s.is_active = false;
            s.user_id = 0;
            s.last_client_seq = 0;
        }
    }

    [[nodiscard]] std::uint32_t assign(std::uint64_t userId) noexcept
    {
        const std::uint32_t id = nextId_.fetch_add(1, std::memory_order_relaxed);
        if (id == 0 || id >= kMax)
            return 0;
        ConnState &s = slots_[id];
        s.user_id = userId;
        s.last_client_seq = 0;
        s.is_active = true;
        {
            std::unique_lock lock(activeByUserMutex_);
            activeByUser_[userId] = id;
        }
        return id;
    }

    void release(std::uint32_t connId) noexcept
    {
        if (connId == 0 || connId >= kMax)
            return;
        ConnState &state = slots_[connId];
        state.is_active = false;
        std::unique_lock lock(activeByUserMutex_);
        const auto it = activeByUser_.find(state.user_id);
        if (it != activeByUser_.end() && it->second == connId)
            activeByUser_.erase(it);
    }

    [[nodiscard]] ConnState &get(std::uint32_t connId) noexcept { return slots_[connId]; }

    [[nodiscard]] std::uint32_t findByUser(std::uint64_t userId) const noexcept
    {
        std::shared_lock lock(activeByUserMutex_);
        const auto it = activeByUser_.find(userId);
        return it == activeByUser_.end() ? 0U : it->second;
    }
};
