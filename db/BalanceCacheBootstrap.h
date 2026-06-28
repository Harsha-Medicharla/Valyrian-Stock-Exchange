#pragma once

#include <cstdint>
#include <string>

class BalanceCache;

void bootstrapBalanceCache(
    BalanceCache &cache,
    const std::string &pgConnString,
    uint32_t numSymbols);
