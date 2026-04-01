#pragma once
#include "crow.h"
#include "db/DatabaseManager.h"

namespace API {
namespace handlers {

class SymbolHandler {
public:
    static crow::response getSymbols();
};

} // namespace handlers
} // namespace API
