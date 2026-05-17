cmake_minimum_required(VERSION 3.14)
project(vse_api_server LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# 1. Find PostgreSQL BEFORE Drogon loads so its configuration scripts can see it natively
find_package(PostgreSQL REQUIRED)

include(FetchContent)

# 2. Keep Drogon features we don't need turned OFF, but force ORM ON for database support
set(BUILD_CTL      OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_ORM      ON  CACHE BOOL "" FORCE)
set(BUILD_YAML_CONFIG OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  drogon_fc
  GIT_REPOSITORY https://github.com/drogonframework/drogon.git
  GIT_TAG e78690747858f49d7ea08002083420ff046d8785
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(drogon_fc)

add_executable(api_server_app
  main_drogon.cpp
  controllers/AuthController.cpp
  controllers/OrderController.cpp
  controllers/BalanceController.cpp
  controllers/AccountController.cpp
  controllers/MarketController.cpp
  middleware/SessionValidator.cpp
  db/PGPool.cpp
  db/RedisPool.cpp
)

target_link_libraries(api_server_app PRIVATE
    Threads::Threads
    drogon
    PostgreSQL::PostgreSQL
    hiredis
    jsoncpp_static
)

# 3. Explicitly append the PostgreSQL include paths so Drogon can find libpq-fe.h anywhere
target_include_directories(api_server_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${PostgreSQL_INCLUDE_DIRS}
)

# Pass the header directory directly into the global compiler flags for third-party sub-builds
include_directories(${PostgreSQL_INCLUDE_DIRS})

target_compile_options(api_server_app PRIVATE -O2 -pthread)
