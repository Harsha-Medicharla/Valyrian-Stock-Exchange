cmake_minimum_required(VERSION 3.14)
project(matching_engine LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- COMMENTED OUT: Handled by Root CMakeLists.txt ---
# include(FetchContent)
# FetchContent_Declare(
#   libart
#   GIT_REPOSITORY https://github.com/armon/libart.git
#   GIT_TAG master
# )
# FetchContent_GetProperties(libart)
# if(NOT libart_POPULATED)
#   FetchContent_MakeAvailable(libart)
#   add_library(art STATIC "${libart_SOURCE_DIR}/src/art.c")
#   target_include_directories(art PUBLIC "${libart_SOURCE_DIR}/src")
#   target_compile_features(art PUBLIC c_std_99)
# endif()
# ----------------------------------------------------

# Define the WAL library
add_library(wal STATIC wal/WAL.cpp)

target_include_directories(
  wal
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  wal
  PUBLIC
  absl::flat_hash_map
  art  # This 'art' now refers to the target defined in the root
)

# Matching Engine Tests
add_executable(
  matching_engine_test
  tests/matching_engine_test.cc
)

target_include_directories(
  matching_engine_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
  matching_engine_test
  PRIVATE
  wal
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(matching_engine_test)