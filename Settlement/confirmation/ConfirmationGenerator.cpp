#include "ConfirmationGenerator.h"
#include <cstdio>

#define BUY 1
#define SELL 2

Confirmation* ConfirmationGenerator::createBuy(Trade* t, FillResult r) {
    auto* c = new Confirmation();

    c->trade_id = t->trade_id;
    c->user_id = t->buyer_id;
    c->order_id = 0;

    snprintf(c->symbol, sizeof(c->symbol), "%lu", t->symbol);

    c->exec_price = t->price;
    c->exec_qty = t->qty;
    c->remaining_qty = 0;

    c->fund_delta = -(t->price * t->qty);
    c->share_delta = t->qty;

    c->side = BUY;
    c->status = r.buy_status;

    return c;
}

Confirmation* ConfirmationGenerator::createSell(Trade* t, FillResult r) {
    auto* c = new Confirmation();

    c->trade_id = t->trade_id;
    c->user_id = t->seller_id;
    c->order_id = 0;

    snprintf(c->symbol, sizeof(c->symbol), "%lu", t->symbol);

    c->exec_price = t->price;
    c->exec_qty = t->qty;
    c->remaining_qty = 0;

    c->fund_delta = (t->price * t->qty);
    c->share_delta = -t->qty;

    c->side = SELL;
    c->status = r.sell_status;

    return c;
}

std::pair<Confirmation*, Confirmation*> 
ConfirmationGenerator::generate(Trade* t, FillResult r) {
    return { createBuy(t, r), createSell(t, r) };
}