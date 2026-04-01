#pragma once
#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <mutex>

namespace API {

class DatabaseManager {
private:
    std::unique_ptr<pqxx::connection> conn_;
    std::mutex mtx_;

    // Private constructor for Singleton
    DatabaseManager() {
        try {
            // Connect to local PostgreSQL instance database 'valyrian'
            conn_ = std::make_unique<pqxx::connection>("dbname=valyrian user=vm1 password=password host=127.0.0.1 port=5432");
            if (conn_->is_open()) {
                std::cout << "[DatabaseManager] Connected successfully to " << conn_->dbname() << std::endl;
            } else {
                std::cerr << "[DatabaseManager] Connection failed." << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "[DatabaseManager] Exception: " << e.what() << std::endl;
        }
    }

public:
    // Delete copy/move semantics for singleton
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    // Thread-safe read transaction
    template <typename Func>
    void executeReadTransaction(Func&& func) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!conn_ || !conn_->is_open()) {
            throw std::runtime_error("Database connection is not open.");
        }
        pqxx::nontransaction tx(*conn_);
        func(tx);
    }
};

} // namespace API
