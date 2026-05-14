cmake_minimum_required(VERSION 3.14)
project(vse_api_server LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

if(DEFINED ENV{HOMEBREW_PREFIX})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{HOMEBREW_PREFIX}/opt/drogon" "$ENV{HOMEBREW_PREFIX}")
endif()
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/drogon" "/usr/local/opt/drogon")

find_package(Drogon CONFIG QUIET)

if(Drogon_FOUND)
  add_executable(api_server_app main_drogon.cpp)
  target_link_libraries(api_server_app PRIVATE Threads::Threads Drogon::Drogon)
else()
  message(STATUS "Drogon not found; building api_server_app as a configure-time stub (install drogon and reconfigure for HTTP server).")
  add_executable(api_server_app main_stub.cpp)
endif()

target_include_directories(api_server_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
)

target_compile_options(api_server_app PRIVATE -O2 -pthread)
