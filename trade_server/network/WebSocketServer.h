#pragma once
#include <App.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include <vector>

#include "core/EMSCore.h"
#include "types/RawOrder.h"
#include "ConnTable.h"
#include "config/TradeServerConfig.h"
#include "resp/IdGenerators.h"

struct redisContext;
struct us_listen_socket_t;
namespace uWS
{
class Loop;
}

class WebSocketServer
{
private:
    EMSCore &ems_;
    ConnTable connTable_;
    std::uint16_t port_;
    std::string redis_host_;
    int redis_port_;
    bool redis_enabled_{true};
    IdGenerator order_ids_;
    std::size_t num_symbols_{0};
    std::unique_ptr<std::atomic<uint64_t>[]> server_seq_by_symbol_;
    struct ConnectionEndpoint
    {
        uWS::Loop *loop{nullptr};
        void *socket{nullptr};
        std::uint64_t generation{0};
    };
    std::vector<ConnectionEndpoint> endpoints_;
    std::mutex endpointsMutex_;
    std::vector<uWS::Loop *> loops_;
    std::vector<us_listen_socket_t *> listenSockets_;
    std::mutex loopMutex_;
    std::atomic<std::uint64_t> nextEndpointGeneration_{1};
    std::atomic<bool> running_{false};
    std::atomic<bool> stopCalled_{false};
    std::atomic<bool> cancelSubscriberRunning_{false};
    std::thread cancelSubscriberThread_;
    std::function<void(std::uint32_t, const std::string &)> sendObserver_;

    [[nodiscard]] bool devSkipAuth() const noexcept;
    [[nodiscard]] bool resolveUserFromBearer(redisContext *redis, std::string_view authorization,
                                             std::uint64_t &out_user) const noexcept;
    [[nodiscard]] bool nextServerSequence(std::uint32_t symbol_id, std::uint64_t &out_seq) noexcept;
    [[nodiscard]] std::uint64_t wallTimestampNs() const noexcept;
    void handleFlatBufferMessage(std::uint64_t user_id, std::uint32_t conn_id, std::string_view message) noexcept;
    void registerEndpoint(std::uint32_t conn_id, uWS::Loop *loop, void *socket) noexcept;
    void unregisterEndpoint(std::uint32_t conn_id) noexcept;
    void registerLoop(uWS::Loop *loop, us_listen_socket_t *listenSocket) noexcept;
    void runCancelSubscriber() noexcept;

public:
    explicit WebSocketServer(EMSCore &ems, std::uint16_t port);
    WebSocketServer(EMSCore &ems, std::uint16_t port, bool redisEnabled);
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
    [[nodiscard]] bool pushCancelOrder(std::uint64_t user_id, std::uint32_t symbol_id,
                                       std::uint64_t order_id) noexcept;

    [[nodiscard]] bool sendToConnection(std::uint32_t conn_id, std::string payload) noexcept;
    void setSendObserver(std::function<void(std::uint32_t, const std::string &)> observer) noexcept
    {
        sendObserver_ = std::move(observer);
    }

    void run();
    void startCancelSubscriber();
    void stopCancelSubscriber() noexcept;
    void stop() noexcept;
};
