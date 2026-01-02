// #include "DataStructures.h"
// #include <bits/stdc++.h>
// using namespace std;

// class OrderBook {
// private:
//   friend class MatchingEngine;
//   OrderPool order_pool;
//   PriceLevelPool price_level_pool;
//   TradeEventPool trade_event_pool;
//   unordered_map<OrderId, Order *> order_index;
//   map<Price, PriceLevel *> buy_book;
//   map<Price, PriceLevel *> sell_book;
//   PriceLevel *best_bid;
//   PriceLevel *best_ask;

// public:
//   OrderBook() : best_bid(nullptr), best_ask(nullptr) {}

// private:
//   void insertOrder(Order *o) {
//     PriceLevel *level = getOrCreatePriceLevel(o->side, o->price);
//     fifoPush(level, o);
//     order_index[o->order_id] = o;
//     if (o->side == Side::BUY) {
//       best_bid = buy_book.rbegin()->second;
//     } else {
//       best_ask = sell_book.begin()->second;
//     }
//   }

//   void removeOrder(Order *o) {
//     PriceLevel *level = getOrCreatePriceLevel(o->side, o->price);
//     fifoRemove(level, o);
//     removePriceLevelIfEmpty(o->side, level->price);
//     order_index.erase(o->order_id);
//     if (o->side == Side::BUY) {
//       if (buy_book.empty()) {
//         best_bid = nullptr;
//       } else {
//         best_bid = buy_book.rbegin()->second;
//       }
//     } else {
//       if (sell_book.empty()) {
//         best_ask = nullptr;
//       } else {
//         best_ask = sell_book.begin()->second;
//       }
//     }
//     order_pool.deallocate(o);
//   }

//   PriceLevel *getOrCreatePriceLevel(Side side, Price price) {
//     auto &book = (side == Side::BUY) ? buy_book : sell_book;

//     auto lvl_it = book.find(price);
//     if (lvl_it != book.end()) {
//       return lvl_it->second;
//     }

//     PriceLevel *level = price_level_pool.allocate();
//     level->price = price;
//     book[price] = level;
//     return level;
//   }

//   void removePriceLevelIfEmpty(Side side, Price price) {
//     auto &book = (side == Side::BUY) ? buy_book : sell_book;

//     auto lvl_it = book.find(price);
//     if (lvl_it == book.end()) {
//       return;
//     }
//     PriceLevel *level = lvl_it->second;
//     if (empty(level)) {
//       book.erase(lvl_it);
//       price_level_pool.deallocate(level);
//     }
//   }

//   Order *findOrder(OrderId order_id) {
//     auto it = order_index.find(order_id);
//     if (it == order_index.end()) {
//       return nullptr;
//     }
//     return it->second;
//   }
// };

#pragma once
#include "AdaptiveRadixTree.h"
#include "DataStructures.h"
#include <unordered_map>

class OrderBook {
private:
  friend class MatchingEngine;
  OrderPool order_pool;
  PriceLevelPool price_level_pool;
  std::unordered_map<OrderId, Order *> order_index;

  ARTreeWrapper buy_book;  // Max-Heap behavior (we want maxTick)
  ARTreeWrapper sell_book; // Min-Heap behavior (we want minTick)

  PriceLevel *best_bid = nullptr;
  PriceLevel *best_ask = nullptr;

  static constexpr uint64_t TICK_MULTIPLIER = 100;

public:

  void insertOrder(Order *o) {
    PriceLevel *level = getOrCreatePriceLevel(o->side, o->price);
    fifoPush(level, o);
    order_index[o->order_id] = o;

    // Update best price pointers
    if (o->side == Side::BUY) {
      if (!best_bid || o->price > best_bid->price)
        best_bid = level;
    } else {
      if (!best_ask || o->price < best_ask->price)
        best_ask = level;
    }
  }

  void removeOrder(Order *o) {
    PriceLevel *level = findPriceLevel(o->side, o->price);
    if (!level)
      return;

    fifoRemove(level, o);
    order_index.erase(o->order_id);

    // ONLY remove from ART if the price level is now empty
    if (level->head == nullptr) {
      uint64_t ticks = static_cast<uint64_t>(o->price * TICK_MULTIPLIER);
      if (o->side == Side::BUY) {
        buy_book.remove(ticks);
        best_bid = buy_book.isEmpty()
                       ? nullptr
                       : static_cast<PriceLevel *>(
                             buy_book.search(buy_book.maxTick()));
      } else {
        sell_book.remove(ticks);
        best_ask = sell_book.isEmpty()
                       ? nullptr
                       : static_cast<PriceLevel *>(
                             sell_book.search(sell_book.minTick()));
      }
      price_level_pool.deallocate(level);
    }
    order_pool.deallocate(o);
  }

  PriceLevel *getOrCreatePriceLevel(Side side, Price price) {
    uint64_t ticks = static_cast<uint64_t>(price * TICK_MULTIPLIER);
    void *existing =
        (side == Side::BUY) ? buy_book.search(ticks) : sell_book.search(ticks);

    if (existing)
      return static_cast<PriceLevel *>(existing);

    PriceLevel *level = price_level_pool.allocate();
    level->price = price;
    level->head = level->tail = nullptr;

    if (side == Side::BUY)
      buy_book.insert(ticks, level);
    else
      sell_book.insert(ticks, level);

    return level;
  }

  PriceLevel *findPriceLevel(Side side, Price price) const {
    uint64_t ticks = static_cast<uint64_t>(price * TICK_MULTIPLIER);
    void *v =
        (side == Side::BUY) ? buy_book.search(ticks) : sell_book.search(ticks);
    return static_cast<PriceLevel *>(v);
  }
  Order *findOrder(OrderId order_id) {
    auto it = order_index.find(order_id);
    if (it == order_index.end()) {
      return nullptr;
    }
    return it->second;
  }
  // Inside OrderBook.h
public:
  OrderBook() : best_bid(nullptr), best_ask(nullptr) {}

  // Add these so the test can access private data
  PriceLevel *getBestBid() const { return best_bid; }
  PriceLevel *getBestAsk() const { return best_ask; }

  // This fixes the "no member getBuyBookSize" error
  size_t getBuyBookSize() const { return buy_book.size(); }
  size_t getSellBookSize() const { return sell_book.size(); }
};