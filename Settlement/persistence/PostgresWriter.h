#pragma once
#include "../entities/Trade.h"
#include <vector>
#include <string>

class PostgresWriter {
public:
    // Initialize with your actual database credentials
    PostgresWriter() : conn_string("dbname=valyrian user=postgres password=root host=127.0.0.1 port=5432") {}
    
    void executeBatch(const std::vector<Trade>& trades);

private:
    std::string conn_string;
};