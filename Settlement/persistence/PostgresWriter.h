#pragma once
#include <libpq-fe.h>
#include <string>
#include <iostream>
#include <cstdint>  // <--- THIS IS THE FIX

namespace SettlementCore {

class PostgresWriter {
public:
    PostgresWriter() {
        const char* conn_info = "dbname=valyrian user=postgres password=root host=127.0.0.1 port=5432";
        conn = PQconnectdb(conn_info);
        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Database Connection Failed: " << PQerrorMessage(conn) << std::endl;
        }
    }

    ~PostgresWriter() {
        if (conn) PQfinish(conn);
    }

    // Now uint64_t and int64_t will be recognized
    void writeTrade(uint64_t buyer, uint64_t seller, uint64_t symbol, int64_t price, int32_t qty) {
        if (!conn || PQstatus(conn) != CONNECTION_OK) return;

        // Constructing the SQL string to match your teammate's order_history schema
        std::string sql = "INSERT INTO order_history (user_id, symbol_id, side, type, status, price, quantity) VALUES "
                          "(" + std::to_string(buyer) + "," + std::to_string(symbol) + ",'BUY','LIMIT','FILLED'," + std::to_string(price/100.0) + "," + std::to_string(qty) + "),"
                          "(" + std::to_string(seller) + "," + std::to_string(symbol) + ",'SELL','LIMIT','FILLED'," + std::to_string(price/100.0) + "," + std::to_string(qty) + ");";

        PGresult* res = PQexec(conn, sql.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "Postgres INSERT Error: " << PQerrorMessage(conn) << std::endl;
        }
        PQclear(res);
    }

private:
    PGconn* conn = nullptr;
};

} // namespace SettlementCore