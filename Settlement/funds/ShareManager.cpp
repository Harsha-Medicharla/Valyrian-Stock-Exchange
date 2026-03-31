#include "ShareManager.h"

void ShareManager::transferShares(uint64_t seller_id, uint64_t buyer_id, 
                                  const char* symbol, int32_t qty) {
    in_memory_holdings[seller_id][symbol].blocked_qty -= qty;
    in_memory_holdings[buyer_id][symbol].available_qty += qty;
}