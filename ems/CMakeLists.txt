cmake_minimum_required(VERSION 3.14)
project(ems LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

FetchContent_Declare(
  disruptorplus
  GIT_REPOSITORY https://github.com/lewissbaker/disruptorplus.git
  GIT_TAG master
)
FetchContent_MakeAvailable(disruptorplus)

# Patch an upstream type-mismatch in lewissbaker/disruptorplus with AppleClang/libc++,
# where std::min(size_t, sequence_t) is ill-formed on some targets.
set(DISRUPTOR_MT_CLAIM_HEADER
  "${disruptorplus_SOURCE_DIR}/include/disruptorplus/multi_threaded_claim_strategy.hpp")
if(EXISTS "${DISRUPTOR_MT_CLAIM_HEADER}")
  file(READ "${DISRUPTOR_MT_CLAIM_HEADER}" _dp_mt_claim_content)
  string(REPLACE
    "std::min(count, static_cast<sequence_t>(diff + 1));"
    "std::min(count, static_cast<size_t>(diff + 1));"
    _dp_mt_claim_content
    "${_dp_mt_claim_content}")
  file(WRITE "${DISRUPTOR_MT_CLAIM_HEADER}" "${_dp_mt_claim_content}")
endif()

find_package(Threads REQUIRED)

add_executable(ems_app
  main.cpp
)

target_include_directories(ems_app
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${rigtorp_spscqueue_SOURCE_DIR}/include
    ${disruptorplus_SOURCE_DIR}/include
)

target_compile_options(ems_app
  PRIVATE
    -O3
    -march=native
    -pthread
)

target_link_libraries(ems_app
  PRIVATE
    Threads::Threads
)
