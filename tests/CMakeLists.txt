cmake_minimum_required(VERSION 3.14)

include(FetchContent)
FetchContent_GetProperties(vse_flatbuffers_headers)

add_executable(vse_trade_flow_test
  integration/TradeFlowTest.cpp
  ${CMAKE_SOURCE_DIR}/trade_server/network/WebSocketServer.cpp
)

# Added ALL missing trade_server subdirectories so WebSocketServer.cpp compiles perfectly
target_include_directories(vse_trade_flow_test PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/trade_server
  ${CMAKE_SOURCE_DIR}/trade_server/network
  ${CMAKE_SOURCE_DIR}/trade_server/protocol
  "${vse_flatbuffers_headers_SOURCE_DIR}/include"
)

target_link_libraries(vse_trade_flow_test PRIVATE
  GTest::gtest_main
  ems_lib
  vse_db_lib
  vse_market_data_lib
  wal
  vse_shared
  uwebsockets_vse
  usockets_vse
  zlibstatic
  hiredis
)

# Ensure the flatbuffer headers are generated BEFORE we compile the test
if(TARGET trade_server_wiretypes)
    add_dependencies(vse_trade_flow_test trade_server_wiretypes)
endif()

add_test(NAME vse_trade_flow_test COMMAND vse_trade_flow_test)
