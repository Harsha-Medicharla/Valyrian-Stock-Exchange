cmake_minimum_required(VERSION 3.14)
# Allow FetchContent subprojects (e.g. hiredis) that still declare CMake < 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(vse LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared headers
add_subdirectory(shared)

# Matching engine (existing, includes WAL)
add_subdirectory(MatchingEngine)

# EMS (existing)
add_subdirectory(ems)

# New modules
add_subdirectory(trade_server)
add_subdirectory(market_data)
add_subdirectory(db)
add_subdirectory(api_server)
