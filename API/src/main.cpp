#include "crow.h"
#include "db/DatabaseManager.h"
#include "handlers/UserHandler.h"
#include "handlers/SymbolHandler.h"
#include "handlers/AuthHandler.h"
#include "middleware/AuthInterceptor.h"
#include <iostream>

using namespace API;
using namespace API::handlers;
using namespace API::middleware;

int main() {
    // Crow App with Authentication Middleware
    crow::App<AuthInterceptor> app;

    // Force DB initialization on boot
    try {
        DatabaseManager::getInstance();
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to database. " << e.what() << std::endl;
    }

    // ==========================================
    // AUTH ROUTES (Public)
    // ==========================================
    CROW_ROUTE(app, "/api/v1/auth/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return AuthHandler::registerUser(req);
    });

    CROW_ROUTE(app, "/api/v1/auth/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return AuthHandler::login(req);
    });

    // ==========================================
    // SYMBOL ROUTES (Public)
    // ==========================================
    CROW_ROUTE(app, "/api/v1/symbols")
    ([]() {
        return SymbolHandler::getSymbols();
    });

    // ==========================================
    // USER ROUTES (Protected by AuthInterceptor)
    // ==========================================
    CROW_ROUTE(app, "/api/v1/user/funds/<int>")
    ([](int user_id) {
        return UserHandler::getFunds(user_id);
    });

    CROW_ROUTE(app, "/api/v1/user/holdings/<int>")
    ([](int user_id) {
        return UserHandler::getHoldings(user_id);
    });

    CROW_ROUTE(app, "/api/v1/user/info/<int>")
    ([](int user_id) {
        return UserHandler::getInfo(user_id);
    });

    CROW_ROUTE(app, "/api/v1/user/history/<int>")
    ([](int user_id) {
        return UserHandler::getHistory(user_id);
    });

    // Boot up the API Gateway
    std::cout << "\n[Crow API Server] Starting up on http://0.0.0.0:3000\n" << std::endl;
    app.port(3000).multithreaded().run();

    return 0;
}
