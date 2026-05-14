#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <absl/container/flat_hash_map.h>

struct SymbolInfo
{
    uint32_t symbol_id;
    std::string ticker;
    int64_t tick_size;
    uint32_t lot_size;
    bool is_active;
};

class SymbolCache
{
private:
    std::vector<SymbolInfo> symbolsById_;
    absl::flat_hash_map<std::string, uint32_t> idsByTicker_;

public:
    void loadFromDB(const std::string &pgConnString);
    void loadFromList(std::vector<SymbolInfo> symbols);
    [[nodiscard]] std::optional<uint32_t> findByTicker(const std::string &ticker) const;
    [[nodiscard]] std::optional<SymbolInfo> findById(uint32_t id) const;
    [[nodiscard]] uint32_t count() const;
    [[nodiscard]] bool isValidTick(uint32_t symbolId, int64_t price) const;
    [[nodiscard]] bool isValidLot(uint32_t symbolId, uint32_t qty) const;
};
