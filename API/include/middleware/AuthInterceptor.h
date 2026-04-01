#pragma once
#include "crow.h"
#include "handlers/AuthHandler.h"

namespace API {
namespace middleware {

struct AuthInterceptor {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // Only protect /api/v1/user/* paths
        if (req.url.find("/api/v1/user/") == 0) {
            std::string auth_header = req.get_header_value("Authorization");
            if (auth_header.empty() || auth_header.find("Bearer ") != 0) {
                res.code = 401;
                res.body = "{\"status\":\"error\",\"message\":\"Missing or invalid Authorization header\"}";
                res.end();
                return;
            }

            std::string token = auth_header.substr(7);
            uint64_t user_id = 0;
            if (!handlers::AuthHandler::validateToken(token, user_id)) {
                res.code = 401;
                res.body = "{\"status\":\"error\",\"message\":\"Unauthorized: Invalid or expired token\"}";
                res.end();
                return;
            }
            
            // We could store user_id in context if needed
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {}
};

} // namespace middleware
} // namespace API
