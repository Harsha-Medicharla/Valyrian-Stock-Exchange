#pragma once
#include "crow.h"
#include "db/DatabaseManager.h"

namespace API {
namespace handlers {

class AuthHandler {
public:
    static crow::response login(const crow::request& req);
    static crow::response registerUser(const crow::request& req);
    
    // Internal helper for token generation/validation
    static std::string generateToken(uint64_t user_id);
    static bool validateToken(const std::string& token, uint64_t& user_id);
};

} // namespace handlers
} // namespace API
