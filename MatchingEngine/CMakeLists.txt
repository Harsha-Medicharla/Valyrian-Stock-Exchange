cmake_minimum_required(VERSION 3.14)
project(matching_engine LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define the WAL library ---
add_library(wal STATIC wal/WAL.cpp)

# --- Set Include Directories ---
# We use PUBLIC so that anything linking to 'wal' (like your tests) 
# also knows where to find these headers.
target_include_directories(
    wal
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # This allows the engine to find SettlementModule.h
    ${CMAKE_SOURCE_DIR}/Settlement/include 
)

# --- Link Dependencies ---
target_link_libraries(
    wal
    PUBLIC
    absl::flat_hash_map
    art
)