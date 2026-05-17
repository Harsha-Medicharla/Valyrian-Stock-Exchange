#include "WebSocketServer.h"

#include <App.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <json/json.h>
#include <memory>
#include <sstream>
#include <thread>
#include <string_view>
#include <utility>
#include <vector>

#include <flatbuffers/flatbuffers.h>
#include <hiredis.h>

#include "WireTypes_generated.h"
#include "types/RawOrder.h"

namespace
{
    [[nodiscard]] std::string_view trim(std::string_view s) noexcept
    {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
            s.remove_prefix(1);
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
            s.remove_suffix(1);
        return s;
    }

    [[nodiscard]] bool starts_with_ci(std::string_view hay, std::string_view needle) noexcept
    {
        if (hay.size() < needle.size())
            return false;
        for (std::size_t i = 0; i < needle.size(); ++i)
        {
            const char a = static_cast<char>(hay[i]);
            const char b = static_cast<char>(needle[i]);
            const char ca = (a >= 'A' && a <= 'Z') ? static_cast<char>(a - 'A' + 'a') : a;
            const char cb = (b >= 'A' && b <= 'Z') ? static_cast<char>(b - 'A' + 'a') : b;
            if (ca != cb)
                return false;
        }
        return true;
    }

    [[nodiscard]] std::string_view bearerToken(std::string_view authorization) noexcept
    {
        const std::string_view prefix = "bearer ";
        if (!starts_with_ci(authorization, prefix))
            return {};
        return trim(authorization.substr(prefix.size()));
    }

    [[nodiscard]] const char *envOr(const char *key, const char *fallback) noexcept
    {
        const char *v = std::getenv(key);
        return (v && v[0]) ? v : fallback;
    }
} // namespace

struct WsUserData
{
    std::uint64_t user_id{0};
    std::uint32_t conn_id{0};
};

using WsSocket = uWS::WebSocket<false, true, WsUserData>;

WebSocketServer::WebSocketServer(EMSCore &ems, std::uint16_t port)
    : WebSocketServer(ems, port, true)
{
}

WebSocketServer::WebSocketServer(EMSCore &ems, std::uint16_t port, bool redisEnabled)
    : ems_(ems),
      connTable_(),
      port_(port),
      redis_host_(envOr("REDIS_HOST", "127.0.0.1")),
      redis_port_(std::atoi(envOr("REDIS_PORT", "6379"))),
      redis_enabled_(redisEnabled),
      order_ids_(1),
      num_symbols_(ems.numSymbols()),
      server_seq_by_symbol_(num_symbols_ > 0 ? std::make_unique<std::atomic<uint64_t>[]>(num_symbols_) : nullptr),
      endpoints_(TradeServerConfig::MAX_CONNS)
{
    if (server_seq_by_symbol_)
    {
        for (std::size_t i = 0; i < num_symbols_; ++i)
            server_seq_by_symbol_[i].store(1, std::memory_order_relaxed);
    }
}

WebSocketServer::~WebSocketServer()
{
    stop();
    stopCancelSubscriber();
    stopBalanceSyncSubscriber();
}

bool WebSocketServer::devSkipAuth() const noexcept
{
    const char *v = std::getenv("VSE_DEV_SKIP_AUTH");
    return v && v[0] && v[0] != '0';
}

bool WebSocketServer::resolveUserFromBearer(redisContext *redis, std::string_view authorization,
                                            std::uint64_t &out_user) const noexcept
{
    out_user = 0;
    if (devSkipAuth())
    {
        out_user = 1;
        return true;
    }
    const std::string_view tok = bearerToken(authorization);
    if (tok.empty() || !redis)
        return false;

    std::string key;
    key.reserve(8 + tok.size());
    key.append("session:");
    key.append(tok.data(), tok.size());

    redisReply *reply = static_cast<redisReply *>(redisCommand(redis, "GET %s", key.c_str()));
    if (!reply)
        return false;
    if (reply->type != REDIS_REPLY_STRING || reply->len <= 0)
    {
        freeReplyObject(reply);
        return false;
    }
    char *end = nullptr;
    const unsigned long long v = std::strtoull(reply->str, &end, 10);
    freeReplyObject(reply);
    if (end == reply->str || v == 0ULL)
        return false;
    out_user = static_cast<std::uint64_t>(v);
    return true;
}

bool WebSocketServer::nextServerSequence(std::uint32_t symbol_id, std::uint64_t &out_seq) noexcept
{
    if (!server_seq_by_symbol_ || symbol_id >= num_symbols_)
        return false;
    out_seq = server_seq_by_symbol_[symbol_id].fetch_add(1, std::memory_order_acq_rel);
    return true;
}

std::uint64_t WebSocketServer::wallTimestampNs() const noexcept
{
    using clock = std::chrono::system_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count());
}

void WebSocketServer::handleFlatBufferMessage(std::uint64_t user_id, std::uint32_t conn_id, std::string_view message) noexcept
{
    if (message.size() > 64 * 1024)
        return;

    flatbuffers::Verifier verifier(reinterpret_cast<const std::uint8_t *>(message.data()), message.size());
    if (!VSE::VerifyRequestBuffer(verifier))
        return;

    const VSE::Request *req = VSE::GetRequest(message.data());
    if (!req)
        return;

    ConnState &cs = connTable_.get(conn_id);

    const std::size_t workers = ems_.numWorkers();
    if (workers == 0)
        return;
    const std::size_t worker_hint = static_cast<std::size_t>(user_id) % workers;

    RawOrder ro{};
    ro.user_id = static_cast<std::uint32_t>(user_id);
    ro.timestamp = wallTimestampNs();

    switch (req->body_type())
    {
    case VSE::RequestBody_NewOrder: {
        const VSE::NewOrder *n = req->body_as_NewOrder();
        if (!n)
            return;
        if (n->symbol_id() >= num_symbols_)
            return;
        const std::uint64_t cseq = n->client_seq();
        if (cseq != 0 && cseq <= cs.last_client_seq)
            return;
        if (cseq != 0)
            cs.last_client_seq = cseq;

        uint64_t seq = 0;
        if (!nextServerSequence(n->symbol_id(), seq))
            return;

        ro.sequence = seq;
        ro.order_id = order_ids_.nextId();
        ro.symbol_id = n->symbol_id();
        ro.price = n->price();
        ro.qty = n->qty();
        ro.side = static_cast<std::uint8_t>(n->side());
        ro.type = static_cast<std::uint8_t>(n->order_type());
        ro.cancel_flag = 0;
        ro.modify_flag = 0;
        (void)pushRawOrder(ro, worker_hint);
        break;
    }
    case VSE::RequestBody_CancelOrder: {
        const VSE::CancelOrder *c = req->body_as_CancelOrder();
        if (!c)
            return;
        if (c->symbol_id() >= num_symbols_)
            return;
        const std::uint64_t cseq = c->client_seq();
        if (cseq != 0 && cseq <= cs.last_client_seq)
            return;
        if (cseq != 0)
            cs.last_client_seq = cseq;

        uint64_t seq = 0;
        if (!nextServerSequence(c->symbol_id(), seq))
            return;

        ro.sequence = seq;
        ro.order_id = c->order_id();
        ro.symbol_id = c->symbol_id();
        ro.price = 0;
        ro.qty = 0;
        ro.side = 0;
        ro.type = 0;
        ro.cancel_flag = 1;
        ro.modify_flag = 0;
        (void)pushRawOrder(ro, worker_hint);
        break;
    }
    case VSE::RequestBody_ModifyOrder: {
        const VSE::ModifyOrder *m = req->body_as_ModifyOrder();
        if (!m)
            return;
        if (m->symbol_id() >= num_symbols_)
            return;
        const std::uint64_t cseq = m->client_seq();
        if (cseq != 0 && cseq <= cs.last_client_seq)
            return;
        if (cseq != 0)
            cs.last_client_seq = cseq;

        uint64_t seq = 0;
        if (!nextServerSequence(m->symbol_id(), seq))
            return;

        ro.sequence = seq;
        ro.order_id = m->order_id();
        ro.symbol_id = m->symbol_id();
        ro.price = m->new_price();
        ro.qty = m->new_qty();
        ro.side = 0;
        ro.type = 0;
        ro.cancel_flag = 0;
        ro.modify_flag = 1;
        (void)pushRawOrder(ro, worker_hint);
        break;
    }
    default:
        break;
    }
}

void WebSocketServer::registerEndpoint(std::uint32_t conn_id, uWS::Loop *loop, void *socket) noexcept
{
    if (conn_id == 0 || conn_id >= endpoints_.size())
        return;
    std::lock_guard lock(endpointsMutex_);
    endpoints_[conn_id] = ConnectionEndpoint{
        loop,
        socket,
        nextEndpointGeneration_.fetch_add(1, std::memory_order_relaxed)};
}

void WebSocketServer::unregisterEndpoint(std::uint32_t conn_id) noexcept
{
    if (conn_id == 0 || conn_id >= endpoints_.size())
        return;
    std::lock_guard lock(endpointsMutex_);
    endpoints_[conn_id] = {};
}

void WebSocketServer::registerLoop(uWS::Loop *loop, us_listen_socket_t *listenSocket) noexcept
{
    std::lock_guard lock(loopMutex_);
    loops_.push_back(loop);
    listenSockets_.push_back(listenSocket);
}

bool WebSocketServer::pushCancelOrder(std::uint64_t user_id, std::uint32_t symbol_id,
                                      std::uint64_t order_id) noexcept
{
    if (symbol_id >= num_symbols_)
        return false;

    std::uint64_t seq = 0;
    if (!nextServerSequence(symbol_id, seq))
        return false;

    RawOrder ro{};
    ro.sequence = seq;
    ro.order_id = order_id;
    ro.user_id = static_cast<std::uint32_t>(user_id);
    ro.symbol_id = symbol_id;
    ro.timestamp = wallTimestampNs();
    ro.cancel_flag = 1;
    ro.modify_flag = 0;
    return pushRawOrder(ro, static_cast<std::size_t>(user_id));
}

void WebSocketServer::runCancelSubscriber() noexcept
{
    while (cancelSubscriberRunning_.load(std::memory_order_acquire))
    {
        timeval timeout{1, 0};
        redisContext *redis = redisConnectWithTimeout(redis_host_.c_str(), redis_port_, timeout);
        if (!redis || redis->err)
        {
            if (redis)
                redisFree(redis);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        (void)redisSetTimeout(redis, timeout);

        redisReply *sub = static_cast<redisReply *>(
            redisCommand(redis, "SUBSCRIBE vse:orders:cancel"));
        if (sub)
            freeReplyObject(sub);

        while (cancelSubscriberRunning_.load(std::memory_order_acquire))
        {
            void *rawReply = nullptr;
            if (redisGetReply(redis, &rawReply) != REDIS_OK)
                break;
            redisReply *reply = static_cast<redisReply *>(rawReply);
            if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3 &&
                reply->element[2]->type == REDIS_REPLY_STRING)
            {
                Json::Value msg;
                Json::CharReaderBuilder reader;
                std::string errs;
                const char *begin = reply->element[2]->str;
                const char *end = begin + reply->element[2]->len;
                const std::unique_ptr<Json::CharReader> parser(reader.newCharReader());
                if (parser->parse(begin, end, &msg, &errs))
                {
                    (void)pushCancelOrder(msg["user_id"].asUInt64(),
                                          msg["symbol_id"].asUInt(),
                                          msg["order_id"].asUInt64());
                }
            }
            if (reply)
                freeReplyObject(reply);
        }

        redisFree(redis);
    }
}

bool WebSocketServer::sendToConnection(std::uint32_t conn_id, std::string payload) noexcept
{
    if (sendObserver_)
        sendObserver_(conn_id, payload);

    ConnectionEndpoint endpoint{};
    {
        std::lock_guard lock(endpointsMutex_);
        if (conn_id == 0 || conn_id >= endpoints_.size())
            return false;
        endpoint = endpoints_[conn_id];
    }

    if (!endpoint.loop || !endpoint.socket)
        return false;

    endpoint.loop->defer([this, conn_id, endpoint, payload = std::move(payload)]() mutable {
        WsSocket *socket = nullptr;
        {
            std::lock_guard lock(endpointsMutex_);
            if (conn_id == 0 || conn_id >= endpoints_.size())
                return;
            const ConnectionEndpoint &current = endpoints_[conn_id];
            if (current.loop != endpoint.loop || current.socket != endpoint.socket ||
                current.generation != endpoint.generation)
                return;
            socket = static_cast<WsSocket *>(current.socket);
        }
        socket->send(payload, uWS::OpCode::BINARY);
    });

    return true;
}

void WebSocketServer::run()
{
    running_.store(true, std::memory_order_release);
    const char *ioThreadsEnv = std::getenv("VSE_IO_THREADS");
    const int configuredThreads = ioThreadsEnv ? std::atoi(ioThreadsEnv) : 0;
    const std::size_t threadCount = std::max<std::size_t>(
        1, configuredThreads > 0 ? static_cast<std::size_t>(configuredThreads)
                                 : TradeServerConfig::IO_THREADS);
    std::vector<std::thread> ioThreads;
    ioThreads.reserve(threadCount);

    for (std::size_t i = 0; i < threadCount; ++i)
    {
        ioThreads.emplace_back([this]() {
            redisContext *redis = nullptr;
            if (redis_enabled_)
            {
                redis = redisConnect(redis_host_.c_str(), redis_port_);
                if (redis && redis->err)
                {
                    redisFree(redis);
                    redis = nullptr;
                }
            }

            uWS::App app;
            app.ws<WsUserData>(
                   "/*",
                   {
                       .compression = uWS::DISABLED,
                       .maxPayloadLength = 64 * 1024,
                       .idleTimeout = 120,
                       .maxBackpressure = 1 * 1024 * 1024,
                       .upgrade =
                           [this, redis](auto *res, auto *req, auto *context) {
                               std::uint64_t uid = 0;
                               const std::string_view auth = req->getHeader("authorization");
                               if (!resolveUserFromBearer(redis, auth, uid))
                               {
                                   res->writeStatus("401 Unauthorized")->end();
                                   return;
                               }

                               const std::uint32_t cid = connTable_.assign(uid);
                               if (cid == 0)
                               {
                                   res->writeStatus("503 Service Unavailable")->end();
                                   return;
                               }

                               res->template upgrade<WsUserData>(
                                   {.user_id = uid, .conn_id = cid},
                                   req->getHeader("sec-websocket-key"),
                                   req->getHeader("sec-websocket-protocol"),
                                   req->getHeader("sec-websocket-extensions"),
                                   context);
                           },
                       .open =
                           [this](auto *ws) {
                               WsUserData *ud = static_cast<WsUserData *>(ws->getUserData());
                               if (!ud || ud->conn_id == 0)
                                   return;
                               registerEndpoint(ud->conn_id, uWS::Loop::get(), ws);
                           },
                       .message =
                           [this](auto *ws, std::string_view message, uWS::OpCode op) {
                               if (op != uWS::OpCode::BINARY && op != uWS::OpCode::TEXT)
                                   return;
                               WsUserData *ud = static_cast<WsUserData *>(ws->getUserData());
                               if (!ud || ud->user_id == 0 || ud->conn_id == 0)
                                   return;
                               handleFlatBufferMessage(ud->user_id, ud->conn_id, message);
                           },
                       .drain = [](auto * /*ws*/) {},
                       .ping = [](auto * /*ws*/, std::string_view) {},
                       .pong = [](auto * /*ws*/, std::string_view) {},
                       .close =
                           [this](auto *ws, int /*code*/, std::string_view /*msg*/) {
                               WsUserData *ud = static_cast<WsUserData *>(ws->getUserData());
                               if (ud && ud->conn_id != 0)
                               {
                                   unregisterEndpoint(ud->conn_id);
                                   connTable_.release(ud->conn_id);
                               }
                           },
                   })
                .listen(
                    "0.0.0.0",
                    static_cast<int>(port_),
                    0,
                    [this, port = port_](us_listen_socket_t *token) {
                        registerLoop(uWS::Loop::get(), token);
                        if (!token)
                        {
                            std::fprintf(stderr, "vse_trade_server: listen failed on port %u\n",
                                         static_cast<unsigned>(port));
                            std::fflush(stderr);
                        }
                    })
                .run();

            if (redis)
                redisFree(redis);
        });
    }

    for (auto &thread : ioThreads)
        thread.join();
    running_.store(false, std::memory_order_release);
}

void WebSocketServer::startCancelSubscriber()
{
    if (!redis_enabled_)
        return;
    cancelSubscriberRunning_.store(true, std::memory_order_release);
    cancelSubscriberThread_ = std::thread(&WebSocketServer::runCancelSubscriber, this);
}

void WebSocketServer::stopCancelSubscriber() noexcept
{
    cancelSubscriberRunning_.store(false, std::memory_order_release);
    if (cancelSubscriberThread_.joinable())
        cancelSubscriberThread_.join();
}

void WebSocketServer::startBalanceSyncSubscriber(BalanceCache &cache) noexcept
{
    balanceSyncRunning_.store(true, std::memory_order_release);
    balanceSyncThread_ = std::thread([this, &cache]() noexcept {
        runBalanceSyncSubscriberImpl(cache);
    });
}

void WebSocketServer::stopBalanceSyncSubscriber() noexcept
{
    balanceSyncRunning_.store(false, std::memory_order_release);
    if (balanceSyncThread_.joinable())
        balanceSyncThread_.join();
}

void WebSocketServer::runBalanceSyncSubscriberImpl(BalanceCache &cache) noexcept
{
    while (balanceSyncRunning_.load(std::memory_order_acquire))
    {
        timeval timeout{1, 0};
        redisContext *redis = redisConnectWithTimeout(
            redis_host_.c_str(), redis_port_, timeout);
        if (!redis || redis->err)
        {
            if (redis)
                redisFree(redis);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        (void)redisSetTimeout(redis, timeout);

        redisReply *sub = static_cast<redisReply *>(
            redisCommand(redis, "SUBSCRIBE vse:balances:sync"));
        if (sub)
            freeReplyObject(sub);

        while (balanceSyncRunning_.load(std::memory_order_acquire))
        {
            void *rawReply = nullptr;
            if (redisGetReply(redis, &rawReply) != REDIS_OK)
                break;
            redisReply *reply = static_cast<redisReply *>(rawReply);
            if (reply && reply->type == REDIS_REPLY_ARRAY &&
                reply->elements >= 3 &&
                reply->element[2]->type == REDIS_REPLY_STRING)
            {
                Json::Value msg;
                Json::CharReaderBuilder reader;
                std::string errs;
                const char *begin = reply->element[2]->str;
                const char *end = begin + reply->element[2]->len;
                const std::unique_ptr<Json::CharReader> parser(
                    reader.newCharReader());
                if (parser->parse(begin, end, &msg, &errs))
                {
                    const std::string type = msg["type"].asString();
                    const uint32_t userId =
                        static_cast<uint32_t>(msg["user_id"].asUInt64());
                    if (type == "Deposit")
                    {
                        const int64_t amount = msg["amount"].asInt64();
                        if (amount > 0)
                            cache.addAvailable(userId, amount);
                    }
                }
            }
            if (reply)
                freeReplyObject(reply);
        }
        redisFree(redis);
    }
}

void WebSocketServer::stop() noexcept
{
    if (stopCalled_.exchange(true, std::memory_order_acq_rel))
        return;
    running_.store(false, std::memory_order_release);
    std::lock_guard lock(loopMutex_);
    for (std::size_t i = 0; i < loops_.size(); ++i)
    {
        uWS::Loop *loop = loops_[i];
        us_listen_socket_t *listenSocket = i < listenSockets_.size() ? listenSockets_[i] : nullptr;
        if (!loop)
            continue;
        loop->defer([loop, listenSocket]() {
            if (listenSocket)
                us_listen_socket_close(0, listenSocket);
        });
    }
}
