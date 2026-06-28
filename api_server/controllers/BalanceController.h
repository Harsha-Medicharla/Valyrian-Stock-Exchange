#pragma once

#include <drogon/HttpController.h>

class BalanceController : public drogon::HttpController<BalanceController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(BalanceController::getBalance, "/v1/balance", drogon::Get, "SessionValidator");
    ADD_METHOD_TO(BalanceController::getHoldings, "/v1/holdings", drogon::Get, "SessionValidator");
    METHOD_LIST_END

    void getBalance(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getHoldings(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
