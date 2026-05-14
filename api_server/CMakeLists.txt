cmake_minimum_required(VERSION 3.14)
project(vse_api_server LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  jsoncpp_fc
  GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
  GIT_TAG 89e2973c754a9c02a49974d839779b151e95afd6
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(jsoncpp_fc)
set(JSONCPP_INCLUDE_DIRS "${jsoncpp_fc_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(JSONCPP_LIBRARIES jsoncpp_static CACHE STRING "" FORCE)

set(BUILD_CTL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_ORM OFF CACHE BOOL "" FORCE)
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
target_link_libraries(api_server_app PRIVATE Threads::Threads drogon hiredis)

target_include_directories(api_server_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
)

target_compile_options(api_server_app PRIVATE -O2 -pthread)
