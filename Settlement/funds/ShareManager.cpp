#include "ShareManager.h"
#include <string>

static std::string symbolToString(uint64_t symbol) {
    return std::to_string(symbol);
}

void ShareManager::unblockSeller(Trade* t) {
    auto& h = holdings[t->seller_id][symbolToString(t->symbol)];

    h.blocked -= t->qty;
}

void ShareManager::transferShares(Trade* t) {

    std::string sym = symbolToString(t->symbol);

    // seller loses shares
    auto& seller = holdings[t->seller_id][sym];
    seller.qty -= t->qty;

    // buyer gains shares
    auto& buyer = holdings[t->buyer_id][sym];
    buyer.qty += t->qty;
}
bool ShareManager::reserveShares(uint64_t user_id, uint64_t symbol, int64_t qty) {
    std::string sym = std::to_string(symbol);
    auto& h = holdings[user_id][sym];

    if (h.qty < qty) return false;

    h.qty -= qty;
    h.blocked += qty;

    return true;
}
void ShareManager::addShares(uint64_t user_id, uint64_t symbol, int64_t qty) {
    std::string sym = std::to_string(symbol);
    auto& h = holdings[user_id][sym];
    h.qty += qty;
}

void ShareManager::releaseShares(uint64_t user_id, uint64_t symbol, int64_t qty) {
    std::string sym = std::to_string(symbol);
    auto& h = holdings[user_id][sym];

    h.blocked -= qty;
    h.qty += qty;
}