cmake_minimum_required(VERSION 3.14)
project(matching_engine LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(art STATIC libs/art/src/art.c)
target_include_directories(art PUBLIC libs/art/src)


include(FetchContent)

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

target_link_libraries(
  matching_engine_test
  PRIVATE
  art
  GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(matching_engine_test)
