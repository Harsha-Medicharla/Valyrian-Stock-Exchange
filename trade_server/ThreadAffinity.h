#pragma once

#include <cstdlib>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace vse::threads
{
    inline int defaultCoreForRole(std::string_view role) noexcept
    {
        if (role == "IOThread 0")
            return 0;
        if (role == "IOThread 1")
            return 2;
        if (role == "EMSThread 0")
            return 4;
        if (role == "EMSThread 1")
            return 6;
        if (role == "Sequencer")
            return 8;
        if (role == "METhread 0")
            return 9;
        if (role == "METhread 1")
            return 10;
        if (role == "RespThread")
            return 11;
        if (role == "FeedPublisher")
            return 12;
        if (role == "RedisHandler")
            return 13;
        if (role == "PGHandler")
            return 14;
        if (role == "OS + Drogon")
            return 15;
        return -1;
    }

    inline std::optional<int> coreForRole(std::string_view role)
    {
        const char *json = std::getenv("VSE_CORE_MAP_JSON");
        if (json && json[0] != '\0')
        {
            const std::regex pattern(
                "\"" + std::string(role) + "\"\\s*:\\s*(-?\\d+)",
                std::regex_constants::ECMAScript);
            std::cmatch match;
            if (std::regex_search(json, match, pattern) && match.size() > 1)
                return std::atoi(match[1].str().c_str());
        }

        const int fallback = defaultCoreForRole(role);
        if (fallback < 0)
            return std::nullopt;
        return fallback;
    }

    inline void pinToCore(int coreId) noexcept
    {
#if defined(__linux__)
        if (coreId < 0)
            return;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(coreId, &cpuset);
        (void)pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#else
        (void)coreId;
#endif
    }
}
