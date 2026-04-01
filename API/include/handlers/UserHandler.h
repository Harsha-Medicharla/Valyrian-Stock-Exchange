#pragma once
#include "crow.h"
#include "db/DatabaseManager.h"

namespace API {
namespace handlers {

class UserHandler {
public:
    static crow::response getFunds(int user_id);
    static crow::response getHoldings(int user_id);
    static crow::response getInfo(int user_id);
    static crow::response getHistory(int user_id);
};

} // namespace handlers
} // namespace API
