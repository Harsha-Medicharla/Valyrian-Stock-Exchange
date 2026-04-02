#pragma once

#include <cstdint>

#include "../entities/Trade.h"
#include "../entities/Confirmation.h"

#include "../funds/FundManager.h"
#include "../funds/ShareManager.h"
#include "../market/SymbolDataUpdater.h"
#include "../confirmation/ConfirmationGenerator.h"
#include "../core/TradeValidator.h"
#include "../core/PartialFillHandler.h"

#include "../../EMS/queue/RingBuffer.h"

namespace SettlementCore {

class Settlement {
public:
    static const size_t QUEUE_SIZE = 1024;

    // ✅ Default constructor (used by tests / EMS)
    Settlement();

    // ✅ Queue-based constructor (real pipeline)
    Settlement(
        RingBuffer<Trade, QUEUE_SIZE>* q4,
        RingBuffer<Confirmation, QUEUE_SIZE>* q5
    );

    // ✅ Main processing loop
    void run();

    // =========================
    // 🔥 TEST / SIMULATION APIs
    // =========================

    void adminDeposit(uint64_t user_id, int64_t amount);
    void adminDepositShares(uint64_t user_id, uint64_t symbol, int64_t qty);

    void settleTrade(
        uint64_t buyer,
        uint64_t seller,
        uint64_t symbol,
        int64_t price,
        int32_t qty
    );

    // =========================
    // 🔥 MATCHING ENGINE API
    // =========================

    void releaseMargin(
        uint64_t user_id,
        uint64_t symbol,
        uint8_t side,
        int64_t price,
        int32_t qty
    );
    bool reserveMargin(
    uint64_t user_id,
    uint64_t symbol,
    uint8_t side,
    int64_t price,
    int32_t qty
    );

private:
    // Core pipeline step
    void processTrade(Trade& trade);

    // Queues
    RingBuffer<Trade, QUEUE_SIZE>* q4;
    RingBuffer<Confirmation, QUEUE_SIZE>* q5;

    // Modules
    TradeValidator validator;
    FundManager fund_manager;
    ShareManager share_manager;
    SymbolDataUpdater market_updater;
    PartialFillHandler partial_handler;
    ConfirmationGenerator confirmation_gen;
};

}