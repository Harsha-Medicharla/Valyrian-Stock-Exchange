#pragma once

#include <drogon/HttpController.h>

class MarketController : public drogon::HttpController<MarketController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(MarketController::listSymbols, "/v1/symbols", drogon::Get);
    ADD_METHOD_TO(MarketController::getSymbol, "/v1/symbols/{1}", drogon::Get);
    ADD_METHOD_TO(MarketController::getTrades, "/v1/symbols/{1}/trades", drogon::Get);
    METHOD_LIST_END

    void listSymbols(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getSymbol(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                   std::string ticker);
    void getTrades(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                   std::string ticker);
};
