cmake_minimum_required(VERSION 3.14)
project(vse_shared LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vse_shared INTERFACE)
target_include_directories(vse_shared INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
