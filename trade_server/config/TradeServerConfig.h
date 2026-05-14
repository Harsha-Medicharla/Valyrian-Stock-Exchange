#pragma once
#include <cstddef>
#include <cstdint>

namespace TradeServerConfig
{
    inline constexpr uint16_t WS_PORT = 9001;
    inline constexpr uint16_t MDP_WS_PORT = 9002;
    inline constexpr std::size_t MAX_CONNS = 65536;  // conn_table capacity
    inline constexpr std::size_t IO_THREADS = 4;
    inline constexpr std::size_t RESP_QUEUE_CAPACITY = 4096;
}
