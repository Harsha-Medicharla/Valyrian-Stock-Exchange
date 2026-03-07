cmake_minimum_required(VERSION 3.14)
project(vse_market_data LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG 565a5149d54930463d58cb0f69b978d439555e66
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

add_library(vse_market_data_lib STATIC
  MarketDataPublisher.cpp
  CandleBuilder.cpp
)

target_include_directories(vse_market_data_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${rigtorp_spscqueue_SOURCE_DIR}/include
)

target_link_libraries(vse_market_data_lib PUBLIC Threads::Threads vse_shared)

add_executable(market_data_app main.cpp)
target_link_libraries(market_data_app PRIVATE vse_market_data_lib)
