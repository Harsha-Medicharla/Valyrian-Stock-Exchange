#pragma once
#include <cstdint>
#include <cstddef>

namespace EMS {

class SymbolRouter {
public:
    size_t route(uint64_t symbol);
};

} // namespace EMS
