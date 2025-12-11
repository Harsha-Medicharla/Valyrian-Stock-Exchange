#include "wal/WAL.h"
#include <cstdio>
#include <iostream>
#include <fstream>

WALSystem::WALSystem(const std::string &path)
    : logFile(path + ".wal"),
      tradeFile(path + ".trades") {}

void WALSystem::writeEntry()
{
    std::ofstream file(logFile, std::ios::binary | std::ios::app);
    if (file.is_open())
    {
        file.write(reinterpret_cast<const char *>(&reusableEntry), sizeof(LogEntry));
    }
}

void WALSystem::logInput(WalAction action, const Order *order)
{
    reusableEntry.action = action;
    reusableEntry.data.order_id = order->order_id;
    reusableEntry.data.user_id = order->user_id;
    reusableEntry.data.side = order->side;
    reusableEntry.data.type = order->type;
    reusableEntry.data.price = order->price;
    reusableEntry.data.quantity = order->quantity;
    reusableEntry.data.remaining = order->remaining;
    reusableEntry.data.timestamp = order->timestamp;
    reusableEntry.data.state = order->state;

    writeEntry();
}

void WALSystem::logModify(OrderId id, Price newPrice, Qty newQty)
{
    reusableEntry.action = WalAction::MODIFY;
    reusableEntry.data.order_id = id;
    reusableEntry.data.price = newPrice;
    reusableEntry.data.quantity = newQty;

    writeEntry();
}

void WALSystem::logCancel(OrderId id)
{
    reusableEntry.action = WalAction::CANCEL;
    reusableEntry.data.order_id = id;

    writeEntry();
}

void WALSystem::logTrade(OrderId aggId, OrderId restId, Price price, Qty qty)
{
    std::ofstream file(tradeFile, std::ios::app);
    if (file.is_open())
    {
        file << aggId << "," << restId << "," << price << "," << qty << "\n";
    }
}

void WALSystem::recover(std::function<void(const LogEntry &)> visitor)
{
    std::ifstream log(logFile, std::ios::binary);
    if (!log.is_open())
    {
        return;
    }

    while (log.read(reinterpret_cast<char *>(&reusableEntry), sizeof(LogEntry)))
    {
        if (visitor)
        {
            visitor(reusableEntry);
        }
    }
}
