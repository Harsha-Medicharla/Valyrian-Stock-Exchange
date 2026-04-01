#include "PostgresWriter.h"
#include <iostream>

void PostgresWriter::executeBatch(const std::vector<Trade>& trades) {
    if (trades.empty()) return;

    try {
        // 1. Establish connection
        pqxx::connection conn(conn_string);
        
        // 2. Start a transaction (Work object)
        pqxx::work tx(conn);

        for (const auto& t : trades) {
            persistTrade(tx, t);
        }

        // 3. Commit everything at once for high performance
        tx.commit();
        
    } catch (const std::exception& e) {
        std::cerr << "[POSTGRES ERROR] Batch failed: " << e.what() << "\n";
        // Transaction automatically rolls back here if not committed
    }
}

void PostgresWriter::persistTrade(pqxx::work& tx, const Trade& t) {
    uint64_t trade_value = t.price * t.qty;

    // Update Buyer's reserved funds
    tx.exec_params(
        "UPDATE balances SET reserved = reserved - $1 WHERE user_id = $2",
        trade_value, t.buyer_id
    );

    // Update Seller's available funds
    tx.exec_params(
        "UPDATE balances SET available = available + $1 WHERE user_id = $2",
        trade_value, t.seller_id
    );

    // Record the trade for the Buyer
    tx.exec_params(
        "INSERT INTO trade_history (user_id, symbol, price, qty, side) VALUES ($1, $2, $3, $4, 'BUY')",
        t.buyer_id, t.symbol, t.price, t.qty
    );

    // Record the trade for the Seller
    tx.exec_params(
        "INSERT INTO trade_history (user_id, symbol, price, qty, side) VALUES ($1, $2, $3, $4, 'SELL')",
        t.seller_id, t.symbol, t.price, t.qty
    );
}