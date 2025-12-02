#include<bits/stdc++.h>
using namespace std;
using OrderId = uint64_t;
using UserId = uint64_t;
using Price = int64_t;
using Qty = int64_t;
using SeqNo = uint64_t;
using TimeStamp = uint64_t;
using Symbol = uint64_t;

enum class Side : uint8_t{
    BUY,SELL
};
enum class OrderState : uint8_t{
    NEW,PARTIALLY_FILLED,FILLED,CANCELLED
};
enum class OrderType : uint8_t{
    LIMIT,MARKET
};
enum class WALEntyrType : uint8_t{
    WAL_NEW_ORDER,WAL_CANCEL_ORDER,WAL_MODIFY_ORDER,WAL_TRADE
};

struct Order{
    OrderId order_id;
    UserId user_id;
    Side side;
    OrderType type;
    Price price;
    Qty quantity;
    Qty remaining;
    TimeStamp timestamp;
    OrderState state;
    Order* prev;
    Order* next;
};

struct PriceLevel{
    Price price;
    Qty aggregated_qty;
    Order* head;
    Order* tail;
    PriceLevel(){
        aggregated_qty = 0;
        head = nullptr;
        tail = nullptr;
    }
};

struct Trade{
    SeqNo trade_id;
    Symbol symbol;
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Qty quantity;
    TimeStamp timestamp;
};


inline void fifoPush(PriceLevel* level,Order* o){
    o->next = nullptr;
    o->prev = level->tail;
    if(level->tail){level->tail->next = o;}
    else{level->head = o;}
    level->tail = o;
    level->aggregated_qty += o->remaining;
}

inline void fifoRemove(PriceLevel* level,Order* o){
    if(o->prev){o->prev->next = o->next;}
    else{level->head = o->next;}
    if(o->next){o->next->prev = o->prev;}
    else{level->tail = o->prev;}
    level->aggregated_qty -= o->remaining;
    o->next = o->prev = nullptr;
}

inline bool empty(PriceLevel* level){
    return level->head == nullptr;
}


static constexpr size_t MAX_ORDERS = 1000000;
struct OrderPool{
    Order pool[MAX_ORDERS];
    Order* free_list[MAX_ORDERS];
    size_t top;
    OrderPool(){
        top = MAX_ORDERS;
        for(size_t i = 0; i < MAX_ORDERS; i++){
            free_list[i] = &pool[i];
        }
    }
    inline Order* allocate(){
        assert(top > 0);
        return free_list[--top];
    }
    inline void deallocate(Order* o){
        assert(top <= MAX_ORDERS);
        free_list[top++] = o;
    }
};

struct PriceLevelPool{
    PriceLevel pool[MAX_ORDERS];
    PriceLevel* free_list[MAX_ORDERS];
    size_t top;
    PriceLevelPool(){
        top = MAX_ORDERS;
        for(size_t i = 0; i < MAX_ORDERS; i++){
            free_list[i] = &pool[i];
        }
    }
    inline PriceLevel* allocate(){
        assert(top > 0);
        return free_list[--top];
    }
    inline void deallocate(PriceLevel* o){
        assert(top <= MAX_ORDERS);
        free_list[top++] = o;
    }
};

struct TradeEventPool{
    Trade pool[MAX_ORDERS];
    Trade* free_list[MAX_ORDERS];
    size_t top;
    TradeEventPool(){
        top = MAX_ORDERS;
        for(int i = 0; i < MAX_ORDERS; i++){
            free_list[i] = &pool[i];
        }
    }
    inline Trade* allocate(){
        assert(top > 0);
        return free_list[--top];
    }
    inline void deallocate(Trade* o){
        assert(top <= MAX_ORDERS);
        free_list[top++] = o;
    }
};
