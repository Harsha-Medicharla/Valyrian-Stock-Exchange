cmake_minimum_required(VERSION 3.14)
project(vse_db LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/libpq")
endif()
find_package(PostgreSQL REQUIRED)

include(FetchContent)
FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG 565a5149d54930463d58cb0f69b978d439555e66
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

add_library(vse_db_lib STATIC
  BalanceCacheBootstrap.cpp
  DBWriter.cpp
  PGWriter.cpp
  SymbolCache.cpp
)

target_include_directories(vse_db_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${PostgreSQL_INCLUDE_DIRS}
  ${rigtorp_spscqueue_SOURCE_DIR}/include
)

target_link_libraries(vse_db_lib PUBLIC
  Threads::Threads
  absl::flat_hash_map
  vse_shared
  PostgreSQL::PostgreSQL
)
