cmake_minimum_required(VERSION 3.14)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
project(vse LANGUAGES C CXX)
if(NOT APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(FETCHCONTENT_QUIET ON CACHE BOOL "" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)


find_package(Threads REQUIRED)
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/libpq")
endif()
find_package(PostgreSQL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS})

include(FetchContent)


FetchContent_Declare(
  hiredis_fc
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG v1.2.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(hiredis_fc)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  jsoncpp_fc
  GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
  GIT_TAG 89e2973c754a9c02a49974d839779b151e95afd6
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(jsoncpp_fc)
set(JSONCPP_INCLUDE_DIRS "${jsoncpp_fc_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(JSONCPP_LIBRARIES jsoncpp_static CACHE STRING "" FORCE)

add_subdirectory(shared)
add_subdirectory(MatchingEngine)
add_subdirectory(ems)
add_subdirectory(market_data)
add_subdirectory(db)
add_subdirectory(api_server)
add_subdirectory(trade_server)
add_subdirectory(tests)
