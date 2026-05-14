#include <cstdlib>
#include <functional>

#include <drogon/HttpResponse.h>
#include <drogon/HttpSimpleController.h>
#include <drogon/drogon.h>

namespace
{
    [[nodiscard]] int listenerPort() noexcept
    {
        if (const char *p = std::getenv("API_PORT"))
        {
            const int v = std::atoi(p);
            if (v > 0 && v < 65536)
                return v;
        }
        return 8080;
    }
} // namespace

class HealthController : public drogon::HttpSimpleController<HealthController>
{
public:
    void asyncHandleHttpRequest(const drogon::HttpRequestPtr &,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback) override
    {
        Json::Value body(Json::objectValue);
        body["status"] = "ok";
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    }

    PATH_LIST_BEGIN
    PATH_ADD("/healthz", drogon::Get);
    PATH_LIST_END
};

class SessionController : public drogon::HttpSimpleController<SessionController>
{
public:
    void asyncHandleHttpRequest(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback) override
    {
        (void)req;
        Json::Value body(Json::objectValue);
        body["token"] = "dev-session-token";
        body["user_id"] = 1;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
    }

    PATH_LIST_BEGIN
    PATH_ADD("/v1/sessions", drogon::Post);
    PATH_LIST_END
};

int main()
{
    drogon::app().addListener("0.0.0.0", listenerPort());
    drogon::app().setLogLevel(trantor::Logger::kInfo);
    drogon::app().run();
    return 0;
}
