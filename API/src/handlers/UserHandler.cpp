#include "handlers/UserHandler.h"
#include <pqxx/pqxx>

namespace API {
namespace handlers {

crow::response UserHandler::getFunds(int user_id) {
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT cash_balance, blocked_funds FROM user_funds WHERE user_id = " + tx.quote(user_id) + ";";
            pqxx::result res = tx.exec(query);

            if (res.empty()) {
                response["status"] = "error";
                response["message"] = "User funds not found";
            } else {
                response["status"] = "success";
                response["data"]["user_id"] = user_id;
                response["data"]["cash_balance"] = res[0]["cash_balance"].as<long long>();
                response["data"]["blocked_funds"] = res[0]["blocked_funds"].as<long long>();
            }
        });
    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
        return crow::response(500, response);
    }
    return crow::response(200, response);
}

crow::response UserHandler::getHoldings(int user_id) {
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT s.symbol, uh.quantity, uh.blocked_qty "
                                "FROM user_holdings uh "
                                "JOIN symbols s ON uh.symbol_id = s.symbol_id "
                                "WHERE uh.user_id = " + tx.quote(user_id) + ";";
            pqxx::result res = tx.exec(query);

            response["status"] = "success";
            response["data"]["user_id"] = user_id;
            
            std::vector<crow::json::wvalue> holdings;
            for (auto row : res) {
                crow::json::wvalue holding;
                holding["symbol"] = row["symbol"].c_str();
                holding["quantity"] = row["quantity"].as<long long>();
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
}

crow::response UserHandler::getInfo(int user_id) {
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT email, name, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at FROM users WHERE user_id = " + tx.quote(user_id) + ";";
            pqxx::result res = tx.exec(query);

            if (res.empty()) {
                response["status"] = "error";
                response["message"] = "User not found";
            } else {
                response["status"] = "success";
                response["data"]["user_id"] = user_id;
                response["data"]["name"] = res[0]["name"].c_str();
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
}

crow::response UserHandler::getHistory(int user_id) {
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT oh.id, s.symbol, oh.side, oh.type, oh.status, oh.price, oh.quantity, "
                                "TO_CHAR(oh.created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at "
                                "FROM order_history oh "
                                "JOIN symbols s ON oh.symbol_id = s.symbol_id "
                                "WHERE oh.user_id = " + tx.quote(user_id) + " ORDER BY oh.created_at DESC;";
            pqxx::result res = tx.exec(query);

            response["status"] = "success";
            response["data"]["user_id"] = user_id;
            
            std::vector<crow::json::wvalue> history;
            for (auto row : res) {
                crow::json::wvalue order;
                order["order_id"] = row["id"].as<long long>();
                order["symbol"] = row["symbol"].c_str();
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
}

} // namespace handlers
} // namespace API
