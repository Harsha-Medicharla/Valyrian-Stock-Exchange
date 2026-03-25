cmake_minimum_required(VERSION 3.14)

project(valyrian_stock_exchange LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------------------------------------------------------
# Adaptive Radix Tree (ART)
# ---------------------------------------------------------
set(ART_SRC_DIR "${CMAKE_SOURCE_DIR}/MatchingEngine/libs/art/src")

add_library(art STATIC "${ART_SRC_DIR}/art.c")
target_include_directories(art PUBLIC "${ART_SRC_DIR}")
target_compile_features(art PUBLIC c_std_99)

# ---------------------------------------------------------
# External Dependencies (Abseil & GoogleTest)
# ---------------------------------------------------------
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
include(GoogleTest)

add_subdirectory(MatchingEngine)
add_subdirectory(EMS)
add_subdirectory(Settlement)
add_subdirectory(Server)

# ---------------------------------------------------------
# Test 1: Pipeline Integration (integration_test.cc)
# ---------------------------------------------------------
add_executable(pipeline_integration_test
    tests/integration_test.cc
)

target_include_directories(pipeline_integration_test PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(pipeline_integration_test
    PRIVATE
        ems
        settlement
        GTest::gtest
        GTest::gtest_main
)

# ---------------------------------------------------------
# Test 2: EMS & Settlement Core (ems_settlement_integration_test.cc)
# ---------------------------------------------------------
add_executable(ems_settlement_test
    tests/ems_settlement_integration_test.cc
)

target_include_directories(ems_settlement_test PRIVATE 
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ems_settlement_test
    PRIVATE
        ems
        settlement
        GTest::gtest
        GTest::gtest_main
)

# ---------------------------------------------------------
# Test 3: Matching Engine (matching_engine_test.cc)
# ---------------------------------------------------------
add_executable(matching_engine_test
    tests/matching_engine_test.cc
)

target_include_directories(matching_engine_test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/MatchingEngine/include
)

target_link_libraries(matching_engine_test
    PRIVATE
        wal
        art
        settlement
        GTest::gtest
        GTest::gtest_main
)

# Discover all tests
gtest_discover_tests(pipeline_integration_test)
gtest_discover_tests(ems_settlement_test)
gtest_discover_tests(matching_engine_test)

# ---------------------------------------------------------
# Test 4: TCP Server Integration Tests
# ---------------------------------------------------------
add_executable(server_integration_test
    tests/server_integration_test.cc
)

target_include_directories(server_integration_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(server_integration_test
    PRIVATE
        server
        ems
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(server_integration_test)