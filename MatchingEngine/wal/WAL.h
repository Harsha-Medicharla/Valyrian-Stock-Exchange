#pragma once

#include <fstream>
#include <functional>
#include <string>
#include <cstdint>

#include "../include/OrderBook.h"

enum class WalAction : uint8_t
{
    ADD = 1,
    CANCEL = 2,
    MODIFY = 3,
    TRADE = 4
};

class OrderData
{
public:
    OrderId order_id;
    UserId user_id;
    Side side;
    OrderType type;
    Price price;
    Qty quantity;
    Qty remaining;
    TimeStamp timestamp;
    OrderState state;
    OrderId peer_order_id = 0;
};

class LogEntry
{
public:
    WalAction action;
    OrderData data;
};

class WALSystem
{
private:
    std::string logFile;   // Path to the log file (unchanged name)

    std::ofstream logStream;

    LogEntry reusableEntry;

    uint64_t lastSeq_{0};

    void writeEntry();

public:
    WALSystem(const std::string &path);

    ~WALSystem();

    void logInput(WalAction action, const Order *order);
    void logModify(OrderId id, Price newPrice, Qty newQty);
    void logCancel(OrderId id);
    void logTrade(OrderId aggId, OrderId restId, Price price, Qty qty);

    void recover(std::function<void(const LogEntry &)> visitor);

    [[nodiscard]] uint64_t lastSequence() const noexcept { return lastSeq_; }
};
