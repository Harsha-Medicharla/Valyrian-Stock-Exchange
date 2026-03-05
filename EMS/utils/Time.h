#pragma once
#include <cstdint>
#include <chrono>

namespace EMS {
namespace utils {

class Time {
public:
    // Placeholder: Grabs the current timestamp in nanoseconds for latency tracking
    static uint64_t nowNanos() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }
};

}
}