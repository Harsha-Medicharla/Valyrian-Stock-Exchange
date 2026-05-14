#pragma once

#include <optional>
#include <string>

class RedisPool
{
private:
    std::string host_{"127.0.0.1"};
    int port_{6379};

public:
    void init(const std::string &host, int port);
    [[nodiscard]] std::optional<std::string> get(const std::string &key) const;
    [[nodiscard]] bool setEx(const std::string &key, const std::string &value, int ttlSeconds) const;
    [[nodiscard]] bool del(const std::string &key) const;
    static RedisPool &instance();
};
