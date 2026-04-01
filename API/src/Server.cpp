#include "crow.h"
#include "DatabaseManager.h"
#include <iostream>

using namespace API;

int main() {
    crow::SimpleApp app;

    // Force DB initialization on boot
    try {
        DatabaseManager::getInstance();
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to database. Make sure PostgreSQL is completely initialized." << std::endl;
    }

    // ==========================================
    // API ROUTE: Get User Funds
    // ==========================================
    CROW_ROUTE(app, "/api/v1/user/funds/<int>")
    ([](int user_id) {
        crow::json::wvalue response;
        try {
            DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
                std::string query = "SELECT available_cash, blocked_cash FROM user_funds WHERE user_id = " + tx.quote(user_id) + ";";
                pqxx::result res = tx.exec(query);

                if (res.empty()) {
                    response["status"] = "error";
                    response["message"] = "User funds not found";
                } else {
                    response["status"] = "success";
                    response["data"]["user_id"] = user_id;
                    response["data"]["available_cash"] = res[0]["available_cash"].as<double>();
                    response["data"]["blocked_cash"] = res[0]["blocked_cash"].as<double>();
                }
            });
        } catch (const std::exception& e) {
            response["status"] = "error";
            response["message"] = e.what();
            return crow::response(500, response);
        }
        return crow::response(200, response);
    });

    // ==========================================
    // API ROUTE: Get User Holdings
    // ==========================================
    CROW_ROUTE(app, "/api/v1/user/holdings/<int>")
    ([](int user_id) {
        crow::json::wvalue response;
        try {
            DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
                // Join USER_HOLDINGS with SYMBOLS to get the ticker name
                std::string query = "SELECT s.ticker, uh.available_qty, uh.blocked_qty "
                                    "FROM user_holdings uh "
                                    "JOIN symbols s ON uh.symbol_id = s.id "
                                    "WHERE uh.user_id = " + tx.quote(user_id) + ";";
                pqxx::result res = tx.exec(query);

                response["status"] = "success";
                response["data"]["user_id"] = user_id;
                
                std::vector<crow::json::wvalue> holdings;
                for (auto row : res) {
                    crow::json::wvalue holding;
                    holding["ticker"] = row["ticker"].c_str();
                    holding["available_qty"] = row["available_qty"].as<long long>();
                    holding["blocked_qty"] = row["blocked_qty"].as<long long>();
                    holdings.push_back(std::move(holding));
                }
                response["data"]["holdings"] = std::move(holdings);
            });
        } catch (const std::exception& e) {
            response["status"] = "error";
            response["message"] = e.what();
            return crow::response(500, response);
        }
        return crow::response(200, response);
    });

    // ==========================================
    // API ROUTE: Get User Info
    // ==========================================
    CROW_ROUTE(app, "/api/v1/user/info/<int>")
    ([](int user_id) {
        crow::json::wvalue response;
        try {
            DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
                std::string query = "SELECT email, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at FROM users WHERE id = " + tx.quote(user_id) + ";";
                pqxx::result res = tx.exec(query);

                if (res.empty()) {
                    response["status"] = "error";
                    response["message"] = "User not found";
                } else {
                    response["status"] = "success";
                    response["data"]["user_id"] = user_id;
                    response["data"]["email"] = res[0]["email"].c_str();
                    response["data"]["created_at"] = res[0]["created_at"].c_str();
                }
            });
        } catch (const std::exception& e) {
            response["status"] = "error";
            response["message"] = e.what();
            return crow::response(500, response);
        }
        return crow::response(200, response);
    });

    // ==========================================
    // API ROUTE: Get Symbols Info
    // ==========================================
    CROW_ROUTE(app, "/api/v1/symbols")
    ([]() {
        crow::json::wvalue response;
        try {
            DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
                std::string query = "SELECT id, ticker, company_name, is_tradable FROM symbols;";
                pqxx::result res = tx.exec(query);

                response["status"] = "success";
                std::vector<crow::json::wvalue> symbols;
                for (auto row : res) {
                    crow::json::wvalue symbol;
                    symbol["id"] = row["id"].as<long long>();
                    symbol["ticker"] = row["ticker"].c_str();
                    symbol["company_name"] = row["company_name"].c_str();
                    symbol["is_tradable"] = row["is_tradable"].as<bool>();
                    symbols.push_back(std::move(symbol));
                }
                response["data"]["symbols"] = std::move(symbols);
            });
        } catch (const std::exception& e) {
            response["status"] = "error";
            response["message"] = e.what();
            return crow::response(500, response);
        }
        return crow::response(200, response);
    });

    // ==========================================
    // API ROUTE: Get Order History
    // ==========================================
    CROW_ROUTE(app, "/api/v1/user/history/<int>")
    ([](int user_id) {
        crow::json::wvalue response;
        try {
            DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
                std::string query = "SELECT oh.id, s.ticker, oh.side, oh.type, oh.status, oh.price, oh.quantity, "
                                    "TO_CHAR(oh.created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at "
                                    "FROM order_history oh "
                                    "JOIN symbols s ON oh.symbol_id = s.id "
                                    "WHERE oh.user_id = " + tx.quote(user_id) + " ORDER BY oh.created_at DESC;";
                pqxx::result res = tx.exec(query);

                response["status"] = "success";
                response["data"]["user_id"] = user_id;
                
                std::vector<crow::json::wvalue> history;
                for (auto row : res) {
                    crow::json::wvalue order;
                    order["order_id"] = row["id"].as<long long>();
                    order["ticker"] = row["ticker"].c_str();
                    order["side"] = row["side"].c_str();
                    order["type"] = row["type"].c_str();
                    order["status"] = row["status"].c_str();
                    order["price"] = row["price"].as<double>();
                    order["quantity"] = row["quantity"].as<long long>();
                    order["created_at"] = row["created_at"].c_str();
                    history.push_back(std::move(order));
                }
                response["data"]["history"] = std::move(history);
            });
        } catch (const std::exception& e) {
            response["status"] = "error";
            response["message"] = e.what();
            return crow::response(500, response);
        }
        return crow::response(200, response);
    });


    // Boot up the API Gateway
    std::cout << "\n[Crow API Server] Starting up on http://0.0.0.0:3000\n" << std::endl;
    app.port(3000).multithreaded().run();

    return 0;
}
