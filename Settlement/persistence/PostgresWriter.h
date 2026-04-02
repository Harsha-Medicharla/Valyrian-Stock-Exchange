#pragma once
#include <libpq-fe.h>
#include <string>
#include <iostream>
#include <cstdint>

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

    void writeTrade(uint64_t trade_id,
                    uint64_t buyer,
                    uint64_t seller,
                    uint64_t symbol,
                    int64_t price,
                    int32_t qty) 
    {
        if (!conn || PQstatus(conn) != CONNECTION_OK) return;

        std::string trade_id_s = std::to_string(trade_id);
        std::string buyer_s = std::to_string(buyer);
        std::string seller_s = std::to_string(seller);
        std::string symbol_s = std::to_string(symbol);
        std::string price_s = std::to_string(price / 100.0);
        std::string qty_s = std::to_string(qty);

        const char* paramValues[6] = {
            trade_id_s.c_str(),
            buyer_s.c_str(),
            seller_s.c_str(),
            symbol_s.c_str(),
            price_s.c_str(),
            qty_s.c_str()
        };
//idempotency
        const char* sql =
            "INSERT INTO trades (trade_id, buyer, seller, symbol, price, qty) "
            "VALUES ($1, $2, $3, $4, $5, $6) "
            "ON CONFLICT (trade_id) DO NOTHING;";

        PGresult* res = PQexecParams(
            conn,
            sql,
            6,
            nullptr,
            paramValues,
            nullptr,
            nullptr,
            0
        );

        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cerr << "Postgres INSERT Error: " << PQerrorMessage(conn) << std::endl;
        }

        PQclear(res);
    }

private:
    PGconn* conn = nullptr;
};

} 