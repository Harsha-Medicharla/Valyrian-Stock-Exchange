#pragma once
#include "../entities/Trade.h"
#include <vector>
#include <string>
#include <pqxx/pqxx> // Ensure libpqxx is installed: sudo apt install libpqxx-dev

class PostgresWriter {
public:
    // Initialize with database credentials
    PostgresWriter() : conn_string("dbname=valyrian user=postgres password=root host=127.0.0.1 port=5432") {}
    
    void executeBatch(const std::vector<Trade>& trades);

private:
    std::string conn_string;
    
    // Internal helper to handle the actual SQL execution
    void persistTrade(pqxx::work& tx, const Trade& t);
};