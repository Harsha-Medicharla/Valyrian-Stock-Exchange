#pragma once
#include <atomic>
#include <cstdint>

namespace EMS {
namespace utils {

class IdGenerator {
public:
    // Generates unique order IDs
    static uint64_t nextOrderId() {
        static std::atomic<uint64_t> current_id{1};
        return current_id.fetch_add(1, std::memory_order_relaxed); 
    }
};

}
}