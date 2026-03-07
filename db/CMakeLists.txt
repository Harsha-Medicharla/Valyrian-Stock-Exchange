cmake_minimum_required(VERSION 3.14)
project(vse_db LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PostgreSQL QUIET)
if(NOT PostgreSQL_FOUND)
  message(WARNING "PostgreSQL not found; skipping db_app target")
  return()
endif()

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  rigtorp_spscqueue
  GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rigtorp_spscqueue)

add_executable(db_app
  DBWriter.cpp
  PGWriter.cpp
)

target_include_directories(db_app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${rigtorp_spscqueue_SOURCE_DIR}/include
  ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(db_app PRIVATE
  Threads::Threads
  vse_shared
  ${PostgreSQL_LIBRARIES}
)
