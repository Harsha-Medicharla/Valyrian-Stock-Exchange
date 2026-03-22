#pragma once
#include <unordered_map>
#include <cstdint>
#include <mutex>
#include <fstream>
#include <sstream>
#include <string>
#include "Types.h"

namespace Settlement {

struct User {
    Price available_cash = 0.0;
    Price blocked_cash = 0.0;
    std::unordered_map<Symbol, Qty> available_stocks;
    std::unordered_map<Symbol, Qty> blocked_stocks;
};

class SettlementModule {
private:
    mutable std::mutex mtx;

public:
    std::unordered_map<UserId, User> users;
    bool reserveMargin(UserId user_id, Symbol symbol, Side side, Price price, Qty qty) {
        std::lock_guard<std::mutex> lock(mtx);
        auto& user = users[user_id];
        if (side == Side::BUY) {
            Price required_cost = price * qty;
            if (user.available_cash < required_cost) return false;
            user.available_cash -= required_cost;
            user.blocked_cash += required_cost;
        } else {
            if (user.available_stocks[symbol] < qty) return false;
            user.available_stocks[symbol] -= qty;
            user.blocked_stocks[symbol] += qty;
        }
        return true;
    }

    void settleTrade(UserId buyer_id, UserId seller_id, Symbol symbol, Price trade_price, Qty traded_qty) {
        std::lock_guard<std::mutex> lock(mtx);
        Price total_value = trade_price * traded_qty;
        users[buyer_id].blocked_cash -= total_value;
        users[buyer_id].available_stocks[symbol] += traded_qty;
        users[seller_id].blocked_stocks[symbol] -= traded_qty;
        users[seller_id].available_cash += total_value;
    }

    void releaseMargin(UserId user_id, Symbol symbol, Side side, Price price, Qty qty) {
        std::lock_guard<std::mutex> lock(mtx);
        auto& user = users[user_id];
        if (side == Side::BUY) {
            Price refund_amount = price * qty;
            user.blocked_cash -= refund_amount;
            user.available_cash += refund_amount;
        } else {
            user.blocked_stocks[symbol] -= qty;
            user.available_stocks[symbol] += qty;
        }
    }


    void saveToDisk(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream file(filename);
        if (!file.is_open()) return;

        for (const auto& [id, user] : users) {
            file << id << "," << user.available_cash << "," << user.blocked_cash << "\n";
        }
        file.close();
    }

    bool loadFromDisk(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mtx);
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        users.clear();
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string item;
            
            UserId id;
            Price avail, blocked;

            if (std::getline(ss, item, ',')) id = std::stoull(item);
            if (std::getline(ss, item, ',')) avail = std::stod(item);
            if (std::getline(ss, item, ',')) blocked = std::stod(item);

            users[id] = {avail, blocked, {}, {}};
        }
        return true;
    }
};

} // namespace Settlement