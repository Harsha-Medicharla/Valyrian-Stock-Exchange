cmake_minimum_required(VERSION 3.10)
project(matching_engine LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(matching_engine MatchingEngine.cpp)

add_library(art STATIC libs/art/src/art.c)

target_include_directories(art PUBLIC libs/art/src)
target_include_directories(matching_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(matching_engine PRIVATE art)
