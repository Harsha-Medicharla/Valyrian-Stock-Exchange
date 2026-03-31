#include "PostgresWriter.h"
#include <iostream>
#include <pqxx/pqxx> 

void PostgresWriter::executeBatch(const std::vector<Trade>& trades) {
    if (trades.empty()) return;

    try {
        // 1. Open Connection
        pqxx::connection C(conn_string);
        
        // 2. Start a Transactional Workspace
        pqxx::work W(C);

        // 3. Loop through trades and build the queries
        for (const auto& t : trades) {
            uint64_t trade_value = t.exec_price * t.exec_qty;

            // Debit blocked cash from buyer
            W.exec_params("UPDATE user_funds SET blocked_cash = blocked_cash - $1 WHERE user_id = $2", 
                          trade_value, t.buy_user_id);
            
            // Credit available cash to seller
            W.exec_params("UPDATE user_funds SET available_cash = available_cash + $1 WHERE user_id = $2", 
                          trade_value, t.sell_user_id);

            // Log the trade in the append-only history table
            // NOTE: We hardcode 'LIMIT' and 'FILLED' here for the mock test
            W.exec_params("INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at) "
                          "VALUES ($1, (SELECT id FROM symbols WHERE ticker = $2), 'BUY', 'LIMIT', 'FILLED', $3, $4, NOW())",
                          t.buy_user_id, t.symbol, t.exec_price, t.exec_qty);
            
            W.exec_params("INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity, created_at) "
                          "VALUES ($1, (SELECT id FROM symbols WHERE ticker = $2), 'SELL', 'LIMIT', 'FILLED', $3, $4, NOW())",
                          t.sell_user_id, t.symbol, t.exec_price, t.exec_qty);
        }

        // 4. Commit the massive batch all at once
        W.commit();
        
        std::cout << "[DB_WORKER] Successfully committed batch of " << trades.size() << " trades to PostgreSQL.\n";

    } catch (const std::exception &e) {
        std::cerr << "[CRITICAL DB ERROR] Failed to flush batch: " << e.what() << std::endl;
    }
}