#include "DBWriter.h"

#include <thread>

void DBWriter::flush()
{
    if (batch_.events().empty())
        return;
    pgWriter_.writeBatch(batch_.events());
    for (const DBEvent &e : batch_.events())
    {
        if (e.type == DBEventType::ORDER_FILLED && e.side == Side::BUY)
        {
            const uint32_t buyUser = static_cast<uint32_t>(e.user_id);
            const uint32_t sellUser = static_cast<uint32_t>(e.peer_user_id);
            balanceCache_.settleTrade(buyUser, sellUser, e.symbol_id, e.fill_price, e.fill_qty);
        }
        else if (e.type == DBEventType::ORDER_CANCELLED)
        {
            if (e.side == Side::BUY)
            {
                const int64_t unblockAmt = e.price * e.remaining;
                balanceCache_.unblockFunds(static_cast<uint32_t>(e.user_id), unblockAmt);
            }
            else
            {
                balanceCache_.unblockHoldings(static_cast<uint32_t>(e.user_id), e.symbol_id,
                                              static_cast<int32_t>(e.remaining));
            }
        }
    }
    batch_.clear();
}

void DBWriter::run()
{
    auto lastFlush = std::chrono::steady_clock::now();
    while (running_.load(std::memory_order_relaxed))
    {
        bool anyWork = false;
        for (uint32_t sym = 0; sym < numSymbols_; ++sym)
        {
            while (DBEvent *ev = dbQueues_[sym].front())
            {
                batch_.push(*ev);
                dbQueues_[sym].pop();
                anyWork = true;
                if (batch_.shouldFlush())
                    flush();
            }
        }
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlush).count();
        if (!batch_.events().empty() && elapsed >= 5)
        {
            flush();
            lastFlush = now;
        }
        if (!anyWork)
            std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    if (!batch_.events().empty())
        flush();
}

void DBWriter::start()
{
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&DBWriter::run, this);
}

void DBWriter::stop() noexcept { running_.store(false, std::memory_order_release); }

void DBWriter::join()
{
    if (thread_.joinable())
        thread_.join();
}
