#include "RedisPool.h"

#include <hiredis.h>

RedisPool &RedisPool::instance()
{
    static RedisPool pool;
    return pool;
}

RedisPool::~RedisPool()
{
    std::lock_guard lock(mutex_);
    closeLocked();
}

void RedisPool::init(const std::string &host, int port)
{
    std::lock_guard lock(mutex_);
    host_ = host;
    port_ = port;
    closeLocked();
    (void)ensureConnectedLocked();
}

void RedisPool::closeLocked() noexcept
{
    if (ctx_)
    {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

bool RedisPool::ensureConnectedLocked() const
{
    if (ctx_ && !ctx_->err)
        return true;

    if (ctx_)
    {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    RedisPool *self = const_cast<RedisPool *>(this);
    self->ctx_ = redisConnect(host_.c_str(), port_);
    if (!self->ctx_ || self->ctx_->err)
    {
        if (self->ctx_)
            redisFree(self->ctx_);
        self->ctx_ = nullptr;
        return false;
    }
    return true;
}

std::optional<std::string> RedisPool::get(const std::string &key) const
{
    std::lock_guard lock(mutex_);
    if (!ensureConnectedLocked())
        return std::nullopt;

    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx_, "GET %s", key.c_str()));
    std::optional<std::string> value;
    if (reply && reply->type == REDIS_REPLY_STRING && reply->str)
        value = std::string(reply->str, static_cast<size_t>(reply->len));
    if (reply)
        freeReplyObject(reply);
    else
        const_cast<RedisPool *>(this)->closeLocked();
    return value;
}

bool RedisPool::setEx(const std::string &key, const std::string &value, int ttlSeconds) const
{
    std::lock_guard lock(mutex_);
    if (!ensureConnectedLocked())
        return false;

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx_, "SET %s %s EX %d", key.c_str(), value.c_str(), ttlSeconds));
    const bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply)
        freeReplyObject(reply);
    else
        const_cast<RedisPool *>(this)->closeLocked();
    return ok;
}

bool RedisPool::del(const std::string &key) const
{
    std::lock_guard lock(mutex_);
    if (!ensureConnectedLocked())
        return false;

    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx_, "DEL %s", key.c_str()));
    const bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply)
        freeReplyObject(reply);
    else
        const_cast<RedisPool *>(this)->closeLocked();
    return ok;
}

bool RedisPool::publish(const std::string &channel, const std::string &message) const
{
    std::lock_guard lock(mutex_);
    if (!ensureConnectedLocked())
        return false;

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx_, "PUBLISH %s %s", channel.c_str(), message.c_str()));
    const bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    if (reply)
        freeReplyObject(reply);
    else
        const_cast<RedisPool *>(this)->closeLocked();
    return ok;
}
