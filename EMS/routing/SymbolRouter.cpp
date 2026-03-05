#include "routing/SymbolRouter.h"

namespace EMS {

uint16_t SymbolRouter::route(const model::OrderRequest& request) {
    if (request.symbol == 1) return 1; 
    return 0; 
}

}