cmake_minimum_required(VERSION 3.14)
project(matching_engine LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(art STATIC libs/art/src/art.c)
target_include_directories(art PUBLIC libs/art/src)


include(FetchContent)

FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20260107.0
)

set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(absl)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()


add_executable(
  matching_engine_test
  tests/matching_engine_test.cc
)

target_include_directories(
  matching_engine_test
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(wal STATIC wal/WAL.cpp)
target_include_directories(wal
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
  wal
  PUBLIC
  absl::flat_hash_map
)

target_link_libraries(
  matching_engine_test
  PRIVATE
  art
  wal
  GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(matching_engine_test)
