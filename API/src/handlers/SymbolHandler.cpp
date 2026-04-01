#include "handlers/SymbolHandler.h"
#include <pqxx/pqxx>

namespace API {
namespace handlers {

crow::response SymbolHandler::getSymbols() {
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT symbol_id, symbol, company_name, exchange FROM symbols;";
            pqxx::result res = tx.exec(query);

            response["status"] = "success";
            std::vector<crow::json::wvalue> symbols;
            for (auto row : res) {
                crow::json::wvalue symbol;
                symbol["symbol_id"] = row["symbol_id"].as<long long>();
                symbol["symbol"] = row["symbol"].c_str();
                symbol["company_name"] = row["company_name"].c_str();
                symbol["exchange"] = row["exchange"].c_str();
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
}

} // namespace handlers
} // namespace API
