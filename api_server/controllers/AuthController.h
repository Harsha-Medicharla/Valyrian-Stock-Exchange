#pragma once

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthController::login, "/v1/sessions", drogon::Post);
    ADD_METHOD_TO(AuthController::logout, "/v1/sessions", drogon::Delete, "SessionValidator");
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void logout(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
