// common/SettlementConfig.h
#pragma once
#include <string>

struct SettlementConfig {
    std::string postgres_conn_string;
    std::string timescale_conn_string;
    size_t q4_pool_size = 100000;
    size_t q5_pool_size = 200000; // Double because 1 trade = 2 confirmations
    int db_flush_interval_ms = 10;
    int db_batch_size = 1000;
};