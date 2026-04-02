#include "Settlement.h"
#include <iostream>

namespace SettlementCore {

Settlement::Settlement()
    : q4(nullptr), q5(nullptr), wal("wal.log") {

    wal.replay([this](const Trade& t) {
        Trade copy = t;
        processTrade(copy, true);
    });

    db_worker.start();
}

Settlement::Settlement(
    RingBuffer<Trade, QUEUE_SIZE>* q4,
    RingBuffer<Confirmation, QUEUE_SIZE>* q5
) : q4(q4), q5(q5), wal("wal.log") {

    wal.replay([this](const Trade& t) {
        Trade copy = t;
        processTrade(copy, true);
    });

    db_worker.start();
}

void Settlement::run() {
    if (!q4 || !q5) return;

    while (true) {
        Trade trade;
        if (!q4->pop(trade)) continue;
        processTrade(trade, false);
    }
}

void Settlement::processTrade(Trade& t, bool from_replay) {

    if (!from_replay) {
        wal.log(t);
    }

    if (!validator.validate(&t)) {
        std::cerr << "Trade validation failed: " << t.trade_id << "\n";
        return;
    }

    fund_manager.unblockBuyer(&t);
    fund_manager.debitBuyer(&t);

    share_manager.unblockSeller(&t);
    share_manager.transferShares(&t);

    fund_manager.creditSeller(&t);

    auto result = partial_handler.handle(&t);

    auto [buy_conf, sell_conf] = confirmation_gen.generate(&t, result);

    if (!from_replay && q5) {
        q5->push(*buy_conf);
        q5->push(*sell_conf);
    }

    delete buy_conf;
    delete sell_conf;

    db_worker.enqueue(t);
}

// TEST HELPERS

void Settlement::adminDeposit(uint64_t user_id, int64_t amount) {
    Trade fake{};
    fake.seller_id = user_id;
    fake.price = amount;
    fake.qty = 1;
    fund_manager.creditSeller(&fake);
}

void Settlement::adminDepositShares(uint64_t user_id, uint64_t symbol, int64_t qty) {
    share_manager.addShares(user_id, symbol, qty);
}

void Settlement::settleTrade(
    uint64_t buyer,
    uint64_t seller,
    uint64_t symbol,
    int64_t price,
    int32_t qty
) {
    static uint64_t next_trade_id = 1;

    Trade t{};
    t.trade_id = next_trade_id++;
    t.buyer_id = buyer;
    t.seller_id = seller;
    t.symbol = symbol;
    t.price = price;
    t.qty = qty;

    t.checksum = t.trade_id ^ buyer ^ seller ^ symbol ^ price ^ qty;

    processTrade(t, false);
}

// EMS

bool Settlement::reserveMargin(
    uint64_t user_id,
    uint64_t symbol,
    uint8_t side,
    int64_t price,
    int32_t qty
) {
    if (side == 1) {
        return fund_manager.reserveFunds(user_id, price * qty);
    } else {
        return share_manager.reserveShares(user_id, symbol, qty);
    }
}

void Settlement::releaseMargin(
    uint64_t user_id,
    uint64_t symbol,
    uint8_t side,
    int64_t price,
    int32_t qty
) {
    if (side == 1) {
        fund_manager.releaseFunds(user_id, price * qty);
    } else {
        share_manager.releaseShares(user_id, symbol, qty);
    }
}

}