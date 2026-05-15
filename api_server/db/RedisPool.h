#pragma once

#include <mutex>
#include <optional>
#include <string>

struct redisContext;

class RedisPool
{
private:
    std::string host_{"127.0.0.1"};
    int port_{6379};
    mutable std::mutex mutex_;
    mutable redisContext *ctx_{nullptr};

    [[nodiscard]] bool ensureConnectedLocked() const;
    void closeLocked() noexcept;

public:
    RedisPool() = default;
    ~RedisPool();

    RedisPool(const RedisPool &) = delete;
    RedisPool &operator=(const RedisPool &) = delete;

    void init(const std::string &host, int port);
    [[nodiscard]] std::optional<std::string> get(const std::string &key) const;
    [[nodiscard]] bool setEx(const std::string &key, const std::string &value, int ttlSeconds) const;
    [[nodiscard]] bool del(const std::string &key) const;
    [[nodiscard]] bool publish(const std::string &channel, const std::string &message) const;
    static RedisPool &instance();
};
