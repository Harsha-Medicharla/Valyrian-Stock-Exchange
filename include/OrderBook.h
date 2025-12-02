#include<bits/stdc++.h>
#include "DataStructures.h"
using namespace std;



class OrderBook{
    private:
    friend class MatchingEngine;
    OrderPool order_pool;
    PriceLevelPool price_level_pool;
    TradeEventPool trade_event_pool;
    unordered_map<OrderId,Order*> order_index;
    map<Price,PriceLevel*> buy_book;
    map<Price,PriceLevel*> sell_book;
    PriceLevel* best_bid;
    PriceLevel* best_ask;

    public:
    OrderBook() : best_bid(nullptr),best_ask(nullptr){}

    private:
    void insertOrder(Order* o){
        PriceLevel* level = getOrCreatePriceLevel(o->side,o->price);
        fifoPush(level,o);
        order_index[o->order_id] = o;
        if(o->side == Side::BUY){best_bid = buy_book.rbegin()->second;}
        else{best_ask = sell_book.begin()->second;}
    }

    void removeOrder(Order* o){
        PriceLevel* level = getOrCreatePriceLevel(o->side,o->price);
        fifoRemove(level,o);
        removePriceLevelIfEmpty(o->side,level->price);
        order_index.erase(o->order_id);
        if(o->side == Side::BUY){
            if(buy_book.empty()){best_bid = nullptr;}
            else{best_bid = buy_book.rbegin()->second;}
        }
        else{
            if(sell_book.empty()){best_ask = nullptr;}
            else{best_ask = sell_book.begin()->second;}
        }
        order_pool.deallocate(o);
    }

    PriceLevel* getOrCreatePriceLevel(Side side,Price price){
        auto& book = (side == Side::BUY) ? buy_book : sell_book;
        
        auto lvl_it = book.find(price);
        if(lvl_it != book.end()){return lvl_it->second;}
        
        PriceLevel* level = price_level_pool.allocate();
        level->price = price;
        book[price] = level;
        return level;
    }

    void removePriceLevelIfEmpty(Side side,Price price){
        auto& book = (side == Side::BUY) ? buy_book : sell_book;

        auto lvl_it = book.find(price);
        if(lvl_it == book.end()){return;}
        PriceLevel* level = lvl_it->second;
        if(empty(level)){
            book.erase(lvl_it);
            price_level_pool.deallocate(level);
        }
    }

    Order* findOrder(OrderId order_id){
        auto it = order_index.find(order_id);
        if(it == order_index.end()){return nullptr;}
        return it->second;
    }
};
