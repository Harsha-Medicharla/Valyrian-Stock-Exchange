cmake_minimum_required(VERSION 3.14)

project(valyrian_stock_exchange LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------------------------------------------------------
# 1. Global Include Path
# ---------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR})

# ---------------------------------------------------------
# 2. Dependencies (PostgreSQL & Threads)
# ---------------------------------------------------------
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS})

# ---------------------------------------------------------
# 3. Adaptive Radix Tree (ART)
# ---------------------------------------------------------
set(ART_SRC_DIR "${CMAKE_SOURCE_DIR}/MatchingEngine/libs/art/src")

add_library(art STATIC "${ART_SRC_DIR}/art.c")
target_include_directories(art PUBLIC "${ART_SRC_DIR}")
target_compile_features(art PUBLIC c_std_99)

# ---------------------------------------------------------
# 4. External Dependencies (Abseil & GoogleTest)
# ---------------------------------------------------------
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20240116.2
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

# ---------------------------------------------------------
# 5. Project Subdirectories
# ---------------------------------------------------------
add_subdirectory(MatchingEngine)
add_subdirectory(EMS)
add_subdirectory(Settlement)

# ---------------------------------------------------------
# Helper Macro for GTests
# ---------------------------------------------------------
macro(add_valyrian_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            ems
            settlement_lib
            art
            wal
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
            ${PostgreSQL_LIBRARIES}
    )
    gtest_discover_tests(${TEST_NAME})
endmacro()

# ---------------------------------------------------------
# 6. Executables & Integration Tests
# ---------------------------------------------------------

# The Smoke Test (To verify DB rows appear)
add_executable(smoke_test tests/smoke_test.cc)
target_link_libraries(smoke_test PRIVATE settlement_lib ${PostgreSQL_LIBRARIES} Threads::Threads)
add_executable(stress_test tests/stress_test.cc)
target_link_libraries(stress_test PRIVATE settlement_lib ${PostgreSQL_LIBRARIES} Threads::Threads)
# The Stress Test (For performance profiling)
# add_executable(stress_test tests/stress_test.cc)
# target_link_libraries(stress_test PRIVATE ems settlement_lib ${PostgreSQL_LIBRARIES} Threads::Threads)

# Standard GTests
add_valyrian_test(pipeline_integration_test tests/integration_test.cc)
add_valyrian_test(ems_settlement_test tests/ems_settlement_integration_test.cc)
add_valyrian_test(matching_engine_test tests/matching_engine_test.cc)