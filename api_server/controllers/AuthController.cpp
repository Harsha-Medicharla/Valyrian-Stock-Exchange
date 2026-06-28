#include "AuthController.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>

#if __has_include(<sodium.h>)
#include <sodium.h>
#define VSE_HAVE_SODIUM 1
#elif __has_include(<bcrypt.h>)
#include <bcrypt.h>
#define VSE_HAVE_BCRYPT 1
#endif

#include "api_server/db/PGPool.h"
#include "api_server/db/RedisPool.h"
#include "api_server/middleware/SessionValidator.h"

namespace
{
    [[nodiscard]] std::string randomTokenHex()
    {
        std::array<unsigned char, 32> bytes{};
        std::ifstream urandom("/dev/urandom", std::ios::binary);
        urandom.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

        std::ostringstream os;
        os << std::hex << std::setfill('0');
        for (unsigned char byte : bytes)
            os << std::setw(2) << static_cast<int>(byte);
        return os.str();
    }

    [[nodiscard]] std::string stubPasswordHash(const std::string &password)
    {
        const std::string salt = randomTokenHex().substr(0, 16);
        const std::size_t digest = std::hash<std::string>{}(salt + ":" + password);
        std::ostringstream os;
        os << "stub$" << salt << "$" << std::hex << digest;
        return os.str();
    }

    [[nodiscard]] bool verifyStubPassword(const std::string &password, const std::string &hash)
    {
        constexpr std::string_view prefix = "stub$";
        if (hash.rfind(std::string(prefix), 0) != 0)
            return false;
        const std::size_t saltEnd = hash.find('$', prefix.size());
        if (saltEnd == std::string::npos)
            return false;
        const std::string salt = hash.substr(prefix.size(), saltEnd - prefix.size());
        const std::size_t digest = std::hash<std::string>{}(salt + ":" + password);
        std::ostringstream os;
        os << "stub$" << salt << "$" << std::hex << digest;
        return os.str() == hash;
    }

    [[nodiscard]] std::string hashPassword(const std::string &password)
    {
#if defined(VSE_HAVE_SODIUM)
        if (sodium_init() < 0)
            return {};
        char out[crypto_pwhash_STRBYTES]{};
        if (crypto_pwhash_str(out, password.c_str(), password.size(),
                              crypto_pwhash_OPSLIMIT_INTERACTIVE,
                              crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
            return {};
        return out;
#elif defined(VSE_HAVE_BCRYPT)
        char salt[BCRYPT_HASHSIZE]{};
        char hash[BCRYPT_HASHSIZE]{};
        if (bcrypt_gensalt(12, salt) != 0)
            return {};
        if (bcrypt_hashpw(password.c_str(), salt, hash) != 0)
            return {};
        return hash;
#else
        return stubPasswordHash(password);
#endif
    }

    [[nodiscard]] bool verifyPassword(const std::string &password, const std::string &hash)
    {
#if defined(VSE_HAVE_SODIUM)
        if (hash.rfind("stub$", 0) == 0)
            return verifyStubPassword(password, hash);
        return sodium_init() >= 0 &&
               crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
#elif defined(VSE_HAVE_BCRYPT)
        if (hash.rfind("stub$", 0) == 0)
            return verifyStubPassword(password, hash);
        return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
#else
        return verifyStubPassword(password, hash);
#endif
    }
}

void AuthController::signup(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto body = req->getJsonObject();
    if (!body || !body->isMember("email") || !body->isMember("password"))
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const std::string email = (*body)["email"].asString();
    const std::string password = (*body)["password"].asString();
    const std::string name = body->isMember("name") ? (*body)["name"].asString() : "";
    if (email.empty() || password.size() < 8)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    const std::string passwordHash = hashPassword(password);
    if (passwordHash.empty())
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
        return;
    }

    try
    {
        const auto db = PGPool::client();
        const auto result = db->execSqlSync(
            "INSERT INTO users (email, password_hash, name) VALUES ($1,$2,$3) "
            "RETURNING user_id",
            email, passwordHash, name);
        const uint64_t newUserId = result[0]["user_id"].as<uint64_t>();
        db->execSqlSync(
            "INSERT INTO balances (user_id, available, blocked) VALUES ($1, 0, 0) "
            "ON CONFLICT (user_id) DO NOTHING",
            newUserId);
        Json::Value out(Json::objectValue);
        out["user_id"] = Json::UInt64(newUserId);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
    }
    catch (const drogon::orm::DrogonDbException &)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k409Conflict);
        callback(resp);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void AuthController::login(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto body = req->getJsonObject();
    if (!body || !body->isMember("email") || !body->isMember("password"))
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try
    {
        const auto db = PGPool::client();
        const auto result = db->execSqlSync(
            "SELECT user_id, password_hash FROM users WHERE email=$1 AND is_active=true",
            (*body)["email"].asString());
        if (result.empty() ||
            !verifyPassword((*body)["password"].asString(), result[0]["password_hash"].as<std::string>()))
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }

        const uint64_t userId = result[0]["user_id"].as<uint64_t>();
        const std::string token = randomTokenHex();
        if (!RedisPool::instance().setEx("session:" + token, std::to_string(userId), 3600))
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        Json::Value out(Json::objectValue);
        out["token"] = token;
        out["user_id"] = Json::UInt64(userId);
        auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
    }
    catch (...)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

void AuthController::logout(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    const auto auth = req->getHeader("Authorization");
    if (const auto token = SessionValidator::bearerToken(auth))
        (void)RedisPool::instance().del("session:" + std::string(*token));

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}
