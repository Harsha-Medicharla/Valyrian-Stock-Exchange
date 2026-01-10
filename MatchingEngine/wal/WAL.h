#ifndef WAL_H
#define WAL_H

#include <fstream>
#include <string>
#include <functional>
#include "../include/OrderBook.h" 

enum class WalAction : uint8_t { ADD = 1, CANCEL = 2, MODIFY = 3 };

struct OrderData {
    OrderId order_id;
    UserId user_id;
    Side side;
    OrderType type;
    Price price;
    Qty quantity;
    Qty remaining;
    TimeStamp timestamp;
    OrderState state;
};

struct LogEntry {
    WalAction action;
    OrderData data;
};

class WALSystem {
private:
    std::string logFile;
    std::string tradeFile;
    LogEntry reusableEntry;

    void writeEntry();

public:
    WALSystem(const std::string& path);

    void logInput(WalAction action, const Order* order);
    void logModify(OrderId id, Price newPrice, Qty newQty);
    void logCancel(OrderId id);
    void logTrade(OrderId aggId, OrderId restId, Price price, Qty qty);

    void recover(std::function<void(const LogEntry&)> visitor);
};

#endif