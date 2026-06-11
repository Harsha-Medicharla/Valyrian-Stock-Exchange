#pragma once

#include "OrderBook.h"

namespace vse::test
{

    struct OrderBookPeer
    {
        static AdaptiveRadixTree &buyBook(OrderBook &ob) noexcept { return ob.buy_book; }
        static AdaptiveRadixTree &sellBook(OrderBook &ob) noexcept { return ob.sell_book; }
        static PriceLevel *&bestBid(OrderBook &ob) noexcept { return ob.best_bid; }
        static PriceLevel *&bestAsk(OrderBook &ob) noexcept { return ob.best_ask; }
        static PriceLevel *getOrCreate(OrderBook &ob, Side side, Price price)
        {
            return ob.getOrCreatePriceLevel(side, price);
        }
        static PriceLevel *getLevel(OrderBook &ob, Side side, Price price) noexcept
        {
            return ob.getPriceLevel(side, price);
        }
        static void removeIfEmpty(OrderBook &ob, Side side, Price price) noexcept
        {
            ob.removePriceLevelIfEmpty(side, price);
        }
    };

}

#include "MatchingEngine.h"

namespace vse::test
{

    struct MatchingEnginePeer
    {
        static OrderBook &orderBook(MatchingEngine &me) noexcept { return me.order_book_; }
        static void updateOrderState(MatchingEngine &me, Order *order) noexcept { me.updateOrderState(order); }
        static void executeTrade(MatchingEngine &me, Order *aggressor, Order *resting, Price price, Qty qty)
        {
            me.executeTrade(aggressor, resting, price, qty);
        }
        static void match(MatchingEngine &me, Order *incoming) noexcept { me.match(incoming, incoming->timestamp); }
    };

}
