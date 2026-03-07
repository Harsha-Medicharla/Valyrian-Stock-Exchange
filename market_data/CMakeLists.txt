cmake_minimum_required(VERSION 3.14)
project(vse_market_data LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

add_executable(market_data_app
  MarketDataPublisher.cpp
  CandleBuilder.cpp
  main.cpp
)

target_include_directories(market_data_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${rigtorp_spscqueue_SOURCE_DIR}/include
)

target_link_libraries(market_data_app PRIVATE Threads::Threads vse_shared)
