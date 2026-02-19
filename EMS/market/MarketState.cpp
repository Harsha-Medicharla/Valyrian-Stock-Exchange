#include "market/MarketState.h"

namespace EMS {

bool MarketState::isSymbolOpen(uint64_t symbol) const {
    return symbol != 0;
}

}