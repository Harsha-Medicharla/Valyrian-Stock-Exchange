#include "RedisPool.h"

#include <hiredis.h>

RedisPool &RedisPool::instance()
{
    static RedisPool pool;
    return pool;
}

void RedisPool::init(const std::string &host, int port)
{
    host_ = host;
    port_ = port;
}

std::optional<std::string> RedisPool::get(const std::string &key) const
{
    redisContext *ctx = redisConnect(host_.c_str(), port_);
    if (!ctx || ctx->err)
    {
        if (ctx)
            redisFree(ctx);
        return std::nullopt;
    }

    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "GET %s", key.c_str()));
    std::optional<std::string> value;
    if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
        value = std::string(reply->str, static_cast<size_t>(reply->len));
    if (reply)
        freeReplyObject(reply);
    redisFree(ctx);
    return value;
}

bool RedisPool::setEx(const std::string &key, const std::string &value, int ttlSeconds) const
{
    redisContext *ctx = redisConnect(host_.c_str(), port_);
    if (!ctx || ctx->err)
    {
        if (ctx)
            redisFree(ctx);
        return false;
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "SET %s %s EX %d", key.c_str(), value.c_str(), ttlSeconds));
    const bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply)
        freeReplyObject(reply);
    redisFree(ctx);
    return ok;
}

bool RedisPool::del(const std::string &key) const
{
    redisContext *ctx = redisConnect(host_.c_str(), port_);
    if (!ctx || ctx->err)
    {
        if (ctx)
            redisFree(ctx);
        return false;
    }

    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "DEL %s", key.c_str()));
    const bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply)
        freeReplyObject(reply);
    redisFree(ctx);
    return ok;
}
