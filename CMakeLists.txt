cmake_minimum_required(VERSION 3.14)
# Allow FetchContent subprojects (e.g. hiredis) that still declare CMake < 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
project(vse LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FETCHCONTENT_QUIET ON CACHE BOOL "" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
  hiredis_fc
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG v1.2.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(hiredis_fc)

# Shared headers
add_subdirectory(shared)

# Matching engine (existing, includes WAL)
add_subdirectory(MatchingEngine)

# EMS (existing)
add_subdirectory(ems)

# New modules
add_subdirectory(market_data)
add_subdirectory(db)
add_subdirectory(api_server)
add_subdirectory(trade_server)
add_subdirectory(tests)
