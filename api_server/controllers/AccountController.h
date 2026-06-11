#pragma once

#include <drogon/HttpController.h>

class AccountController : public drogon::HttpController<AccountController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AccountController::getAccount, "/v1/account", drogon::Get, "SessionValidator");
    ADD_METHOD_TO(AccountController::updateAccount, "/v1/account", drogon::Put, "SessionValidator");
    ADD_METHOD_TO(AccountController::deposit, "/v1/account/deposit", drogon::Post, "SessionValidator");
    ADD_METHOD_TO(AccountController::depositHoldings,
                  "/v1/account/deposit-holdings",
                  drogon::Post, "SessionValidator");
    METHOD_LIST_END

    void getAccount(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void updateAccount(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void deposit(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void depositHoldings(const drogon::HttpRequestPtr &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
