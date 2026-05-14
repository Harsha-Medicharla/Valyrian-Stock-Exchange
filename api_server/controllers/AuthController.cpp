#include "AuthController.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

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

[[nodiscard]] bool verifyPassword(const std::string &password, const std::string &hash)
{
#if defined(VSE_HAVE_SODIUM)
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
#elif defined(VSE_HAVE_BCRYPT)
    return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
#else
    (void)password;
    (void)hash;
    return false;
#endif
}
} // namespace

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
