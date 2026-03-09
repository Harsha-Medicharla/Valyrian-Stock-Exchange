cmake_minimum_required(VERSION 3.14)
project(vse_trade_server LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/libpq")
endif()
find_package(PostgreSQL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG 565a5149d54930463d58cb0f69b978d439555e66
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

FetchContent_Declare(
  disruptorplus
  GIT_REPOSITORY https://github.com/lewissbaker/disruptorplus.git
  GIT_TAG 748e354476e0bf86dfdbb55f1d925222d7ee631b
)
FetchContent_MakeAvailable(disruptorplus)

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

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UsocketsUWebSockets.cmake)

FetchContent_Declare(
  vse_flatbuffers_headers
  GIT_REPOSITORY https://github.com/google/flatbuffers.git
  GIT_TAG v24.3.25  # Force version match with Docker/Ubuntu 24.04
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(vse_flatbuffers_headers)
if(NOT vse_flatbuffers_headers_POPULATED)
  FetchContent_Populate(vse_flatbuffers_headers)
endif()

add_executable(trade_server_app
  main.cpp
  network/WebSocketServer.cpp
)

target_include_directories(trade_server_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/network
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${CMAKE_SOURCE_DIR}/db
  ${CMAKE_SOURCE_DIR}/market_data
  ${rigtorp_spscqueue_SOURCE_DIR}/include
  ${disruptorplus_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/MatchingEngine/include
  ${CMAKE_CURRENT_SOURCE_DIR}/protocol
  "${vse_flatbuffers_headers_SOURCE_DIR}/include"
)

target_link_libraries(trade_server_app PRIVATE
  Threads::Threads
  PostgreSQL::PostgreSQL
  wal
  vse_shared
  vse_db_lib
  vse_market_data_lib
  uwebsockets_vse
  hiredis
  jsoncpp_static
)

target_compile_definitions(trade_server_app PRIVATE VSE_HAVE_HIREDIS=1)

target_compile_options(trade_server_app PRIVATE -O2 -pthread)

set(WIRE_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/protocol/WireTypes.fbs")
set(WIRE_GEN "${CMAKE_CURRENT_SOURCE_DIR}/protocol/WireTypes_generated.h")
find_program(FLATC flatc)
if(FLATC)
  add_custom_command(
    OUTPUT "${WIRE_GEN}"
    COMMAND "${FLATC}" --cpp -o "${CMAKE_CURRENT_SOURCE_DIR}/protocol" "${WIRE_SCHEMA}"
    DEPENDS "${WIRE_SCHEMA}"
    COMMENT "flatc WireTypes.fbs (optional)"
    VERBATIM)
  add_custom_target(trade_server_wiretypes DEPENDS "${WIRE_GEN}")
  add_dependencies(trade_server_app trade_server_wiretypes)
endif()
