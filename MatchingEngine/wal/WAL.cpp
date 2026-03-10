#include "wal/WAL.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>

WALSystem::WALSystem(const std::string &path)
    : logFile(path + ".wal")
{
  logStream.open(logFile, std::ios::binary | std::ios::app);
  if (!logStream.is_open())
  {
    throw std::runtime_error("CRITICAL: Failed to open WAL file: " + logFile);
  }
}

WALSystem::~WALSystem()
{
  if (logStream.is_open())
  {
    logStream.flush();
    logStream.close();
  }
}

void WALSystem::writeEntry()
{
    if (!logStream.good())
        throw std::runtime_error("WAL Stream is in a bad state before write.");

    logStream.write(reinterpret_cast<const char *>(&reusableEntry),
                    sizeof(LogEntry));
    if (logStream.fail())
    {
        logStream.clear();
        throw std::runtime_error("CRITICAL: Failed to write to WAL (Disk Full?).");
    }

    logStream.flush();
    if (logStream.fail())
    {
        logStream.clear();
        throw std::runtime_error("CRITICAL: Failed to flush WAL.");
    }

    ++lastSeq_;
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
  reusableEntry.data.peer_order_id = 0;

  writeEntry();
}

void WALSystem::logModify(OrderId id, Price newPrice, Qty newQty)
{
  reusableEntry.action = WalAction::MODIFY;
  reusableEntry.data.order_id = id;
  reusableEntry.data.peer_order_id = 0;
  reusableEntry.data.price = newPrice;
  reusableEntry.data.quantity = newQty;
  reusableEntry.data.remaining = 0;
  reusableEntry.data.timestamp = 0;

  writeEntry();
}

void WALSystem::logCancel(OrderId id)
{
  reusableEntry.action = WalAction::CANCEL;
  reusableEntry.data.order_id = id;
  reusableEntry.data.peer_order_id = 0;
  reusableEntry.data.price = 0;
  reusableEntry.data.quantity = 0;
  reusableEntry.data.remaining = 0;
  reusableEntry.data.timestamp = 0;

  writeEntry();
}

void WALSystem::logTrade(OrderId aggId, OrderId restId, Price price, Qty qty)
{
  reusableEntry.action = WalAction::TRADE;
  reusableEntry.data.order_id = aggId;
  reusableEntry.data.peer_order_id = restId;
  reusableEntry.data.price = price;
  reusableEntry.data.quantity = qty;
  reusableEntry.data.remaining = 0;
  reusableEntry.data.timestamp = 0;
  reusableEntry.data.state = OrderState::NEW;

  std::string tradeFile = logFile.substr(0, logFile.find_last_of('.')) + ".trades";
  std::ofstream tradeStream(tradeFile, std::ios::binary | std::ios::app);
  if (tradeStream.is_open())
  {
    tradeStream << aggId << "," << restId << "," << price << "," << qty << "\n";
  }
}

void WALSystem::recover(std::function<void(const LogEntry &)> visitor)
{
  lastSeq_ = 0;
  // Use a local stream for reading
  std::ifstream logReader(logFile, std::ios::binary);

  if (!logReader.is_open())
  {
    std::cout << "No existing WAL file found (" << logFile
              << "). Starting fresh." << std::endl;
    return;
  }

  std::cout << "Recovering from WAL..." << std::endl;
  size_t count = 0;

  // read carefully to detect partial writes
  while (true)
  {
    logReader.read(reinterpret_cast<char *>(&reusableEntry), sizeof(LogEntry));

    if (!logReader)
    {
      if (logReader.eof())
      {
        // checks if we read partial bytes
        if (logReader.gcount() > 0)
        {
          std::cerr << "CRITICAL WARNING: Corrupted WAL entry at end of file. "
                    << "Read " << logReader.gcount() << " bytes, expected "
                    << sizeof(LogEntry) << ". Discarding partial record."
                    << std::endl;
        }
        break;
      }

      if (logReader.fail())
      {
        std::cerr << "Error reading WAL file." << std::endl;
        break;
      }
    }

    // Pass the entry to the Matching Engine
    if (visitor)
    {
      try
      {
        visitor(reusableEntry);
        ++lastSeq_;
        count++;
      }
      catch (const std::exception &e)
      {
        std::cerr << "Error processing WAL entry #" << count << ": " << e.what()
                  << std::endl;
        throw;
      }
    }
  }

  std::cout << "Recovery complete. Processed " << count << " entries."
            << std::endl;
}
