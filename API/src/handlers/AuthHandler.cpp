#include "handlers/AuthHandler.h"
#include <pqxx/pqxx>
#include <chrono>
#include <random>
#include <unordered_map>

namespace API {
namespace handlers {

// Simple in-memory session store for now
static std::unordered_map<std::string, uint64_t> sessions;

crow::response AuthHandler::registerUser(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");
    
    std::string email = x["email"].s();
    std::string password = x["password"].s();
    std::string name = x["name"].s();
    
    crow::json::wvalue response;
    try {
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            // Check if user exists
            std::string check = "SELECT user_id FROM users WHERE email = " + tx.quote(email) + ";";
            if (!tx.exec(check).empty()) {
                throw std::runtime_error("User already exists");
            }
        });

        std::string password_hash = password; // PLACEHOLDER

        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {

             std::string query = "INSERT INTO users (name, email, password_hash) VALUES (" + 
                                tx.quote(name) + ", " + tx.quote(email) + ", " + tx.quote(password_hash) + ") RETURNING user_id;";
             pqxx::result res = tx.exec(query);
             
             uint64_t user_id = res[0][0].as<uint64_t>();
             
             // Also initialize user_funds
             tx.exec("INSERT INTO user_funds (user_id, cash_balance) VALUES (" + std::to_string(user_id) + ", 100000000);"); // 10k default
             
             response["status"] = "success";
             response["data"]["user_id"] = user_id;
             response["message"] = "User registered successfully";
        });
        
    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
        return crow::response(500, response);
    }
    return crow::response(200, response);
}

crow::response AuthHandler::login(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");
    
    std::string email = x["email"].s();
    std::string password = x["password"].s();
    
    crow::json::wvalue response;
    try {
        uint64_t user_id = 0;
        bool authenticated = false;
        
        DatabaseManager::getInstance().executeReadTransaction([&](pqxx::nontransaction& tx) {
            std::string query = "SELECT user_id, password_hash FROM users WHERE email = " + tx.quote(email) + ";";
            pqxx::result res = tx.exec(query);
            
            if (!res.empty() && res[0]["password_hash"].c_str() == password) {
                user_id = res[0]["user_id"].as<uint64_t>();
                authenticated = true;
            }
        });
        
        if (authenticated) {
            std::string token = generateToken(user_id);
            sessions[token] = user_id;
            
            response["status"] = "success";
            response["data"]["token"] = token;
            response["data"]["user_id"] = user_id;
        } else {
            response["status"] = "error";
            response["message"] = "Invalid credentials";
            return crow::response(401, response);
        }
        
    } catch (const std::exception& e) {
        response["status"] = "error";
        response["message"] = e.what();
        return crow::response(500, response);
    }
    return crow::response(200, response);
}

std::string AuthHandler::generateToken(uint64_t user_id) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
    
    std::string token = "VAL-";
    for (int i = 0; i < 32; ++i) token += charset[dist(rng)];
    return token;
}

bool AuthHandler::validateToken(const std::string& token, uint64_t& user_id) {
    auto it = sessions.find(token);
    if (it != sessions.end()) {
        user_id = it->second;
        return true;
    }
    return false;
}

} // namespace handlers
} // namespace API
