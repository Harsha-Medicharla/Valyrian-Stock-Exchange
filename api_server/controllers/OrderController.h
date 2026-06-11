#pragma once

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(OrderController::listOrders, "/v1/orders", drogon::Get, "SessionValidator");
    ADD_METHOD_TO(OrderController::cancelOrder, "/v1/orders/{1}", drogon::Delete, "SessionValidator");
    METHOD_LIST_END

    void listOrders(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void cancelOrder(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                     uint64_t orderId);
};
