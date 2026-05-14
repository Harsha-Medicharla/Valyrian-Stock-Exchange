#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "core/EMSCore.h"
#include "types/RawOrder.h"
#include "ConnTable.h"
#include "config/TradeServerConfig.h"
#include "resp/IdGenerators.h"

struct redisContext;

class WebSocketServer
{
private:
    EMSCore &ems_;
    ConnTable connTable_;
    std::uint16_t port_;
    std::string redis_host_;
    int redis_port_;
    redisContext *redis_{nullptr};
    IdGenerator order_ids_;
    std::size_t num_symbols_{0};
    std::unique_ptr<std::atomic<uint64_t>[]> server_seq_by_symbol_;

    [[nodiscard]] bool devSkipAuth() const noexcept;
    [[nodiscard]] bool resolveUserFromBearer(std::string_view authorization, std::uint64_t &out_user) const noexcept;
    [[nodiscard]] bool nextServerSequence(std::uint32_t symbol_id, std::uint64_t &out_seq) noexcept;
    [[nodiscard]] std::uint64_t wallTimestampNs() const noexcept;
    void handleFlatBufferMessage(std::uint64_t user_id, std::uint32_t conn_id, std::string_view message) noexcept;

public:
    explicit WebSocketServer(EMSCore &ems, std::uint16_t port);
    ~WebSocketServer();

    WebSocketServer(const WebSocketServer &) = delete;
    WebSocketServer &operator=(const WebSocketServer &) = delete;

    [[nodiscard]] bool pushRawOrder(const RawOrder &order, std::size_t worker_hint) noexcept
    {
        const std::size_t n = ems_.numWorkers();
        if (n == 0)
            return false;
        return ems_.spscQueue(worker_hint % n).enqueue(order);
    }

    [[nodiscard]] ConnTable &connTable() noexcept { return connTable_; }

    void run();
};
