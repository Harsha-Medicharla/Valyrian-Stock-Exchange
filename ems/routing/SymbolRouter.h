#pragma once
#include <vector>

class SymbolRouter
{
private:
    std::vector<size_t> map_;

public:
    inline explicit SymbolRouter(size_t numSymbols) : map_(numSymbols)
    {
        for (size_t i = 0; i < numSymbols; ++i)
        {
            map_[i] = i;
        }
    }

    inline size_t route(uint32_t symbol_id) const
    {
        return map_[symbol_id];
    }
};
