cmake_minimum_required(VERSION 3.10)
project(order_book)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matching_engine matching_engine.cpp)

target_include_directories(matching_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
