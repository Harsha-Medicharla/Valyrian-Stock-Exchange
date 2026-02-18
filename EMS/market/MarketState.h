#pragma once
#include <cstdint>

namespace EMS {

class MarketState {
public:
    bool isSymbolOpen(uint64_t symbol) const;
};

}
