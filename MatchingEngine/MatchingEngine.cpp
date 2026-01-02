#include "../include/AdaptiveRadixTree.h"
#include "../include/OrderBook.h"
#include <bits/stdc++.h>
using namespace std;

extern "C" {
#include "../libs/art/src/art.h"
}

class MatchingEngine {
private:
  Symbol symbol;
  TimeStamp time_stamp;
  SeqNo trade_seq_no;
  OrderBook order_book;
  // WAL* wal;
public:
  MatchingEngine(Symbol symbol)
      : symbol(symbol), time_stamp(0), trade_seq_no(0) {}

  void onNewOrder(OrderId order_id, UserId user_id, Side side,
                  OrderType order_type, Price price, Qty qty) {
    Order *o = order_book.order_pool.allocate();
    *o = {order_id,     user_id,         side,    order_type, price, qty, qty,
          time_stamp++, OrderState::NEW, nullptr, nullptr};

    match(o);

    updateOrderState(o);

    if (o->type == OrderType::MARKET) {
      return;
    }

    if (o->state == OrderState::NEW or
        o->state == OrderState::PARTIALLY_FILLED) {
      order_book.insertOrder(o);
    }
  }

  void onCancelOrder(OrderId order_id) {
    Order *o = order_book.findOrder(order_id);
    if (o == nullptr) {
      return;
    }
    if (o->state == OrderState::FILLED) {
      return;
    }
    order_book.removeOrder(o);
  }

  void onModifyOrder(OrderId order_id, Price new_price, Qty new_qty) {
    Order *o = order_book.findOrder(order_id);
    if (o == nullptr) {
      return;
    }
    UserId user_id = o->user_id;
    Side side = o->side;
    OrderType order_type = o->type;
    onCancelOrder(order_id);
    onNewOrder(order_id, user_id, side, order_type, new_price, new_qty);
  }

private:
  void match(Order *incoming) {
    auto &book = (incoming->side == Side::BUY) ? order_book.sell_book
                                               : order_book.buy_book;

    while (incoming->remaining > 0 && !book.isEmpty()) {
      PriceLevel *level = (incoming->side == Side::BUY) ? order_book.best_ask
                                                        : order_book.best_bid;
      Price best_price = level->price;
      if (incoming->type == OrderType::LIMIT) {
        if (incoming->side == Side::BUY && incoming->price < best_price) {
          break;
        }
        if (incoming->side == Side::SELL && incoming->price > best_price) {
          break;
        }
      }
      Order *resting = level->head;
      Qty traded = min(incoming->remaining, resting->remaining);

      incoming->remaining -= traded;
      resting->remaining -= traded;
      level->aggregated_qty -= traded;

      updateOrderState(resting);

      if (resting->state == OrderState::FILLED) {
        order_book.removeOrder(resting);
      }
    }
  }

  void executeTrade(Order *aggressor, Order *resting, Price price, Qty qty) {}

  void updateOrderState(Order *o) {
    if (o->remaining == o->quantity) {
      o->state = OrderState::NEW;
    } else if (o->remaining == 0) {
      o->state = OrderState::FILLED;
    } else if (o->remaining > 0 and o->remaining < o->quantity) {
      o->state = OrderState::PARTIALLY_FILLED;
    }
  }
};

int main() {
  MatchingEngine *me1 = new MatchingEngine(0);
  me1->onNewOrder(1, 1, Side::SELL, OrderType::LIMIT, 100, 100);
  me1->onNewOrder(0, 0, Side::BUY, OrderType::MARKET, 100, 100);
  me1->onNewOrder(3, 3, Side::SELL, OrderType::LIMIT, 100, 100);
  me1->onNewOrder(2, 2, Side::BUY, OrderType::MARKET, 100, 100);

  art_tree tree;
  art_tree_init(&tree);

  uint64_t key = 10125; // example price tick
  art_insert(&tree, (unsigned char *)&key, 8, (void *)"TEST_VALUE");

  art_leaf *minLeaf = art_minimum(&tree);
  art_leaf *maxLeaf = art_maximum(&tree);

  std::cout << "Tree size: " << tree.size << "\n";
  std::cout << "Min value: " << (char *)minLeaf->value << "\n";
  std::cout << "Max value: " << (char *)maxLeaf->value << "\n";
  return 0;
}
