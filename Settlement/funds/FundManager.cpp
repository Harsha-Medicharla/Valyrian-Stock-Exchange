#include "FundManager.h"

void FundManager::unblockBuyer(Trade* t) {
    auto& f = funds[t->buyer_id];

    int64_t amount = t->price * t->qty;
    f.blocked -= amount;
}

void FundManager::debitBuyer(Trade* t) {
    auto& f = funds[t->buyer_id];

    int64_t amount = t->price * t->qty;
    f.balance -= amount;
}

void FundManager::creditSeller(Trade* t) {
    auto& f = funds[t->seller_id];

    int64_t amount = t->price * t->qty;
    f.balance += amount;
}
bool FundManager::reserveFunds(uint64_t user_id, int64_t amount) {
    auto& f = funds[user_id];

    if (f.balance < amount) return false;

    f.balance -= amount;
    f.blocked += amount;

    return true;
}

void FundManager::releaseFunds(uint64_t user_id, int64_t amount) {
    auto& f = funds[user_id];

    f.blocked -= amount;
    f.balance += amount;
}