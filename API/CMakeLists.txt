cmake_minimum_required(VERSION 3.14)
project(APIServer LANGUAGES CXX)

# Require C++17 for Crow and generic standard libraries
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find system installation of libpqxx (PostgreSQL C++ client API)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libpqxx REQUIRED libpqxx)

# Fetch latest version of Crow (lightweight HTTP framework)
include(FetchContent)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(Crow)

# Define our API Server executable
add_executable(APIServer
    src/Server.cpp
)

# Include directories
target_include_directories(APIServer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${libpqxx_INCLUDE_DIRS}
)

# Link Crow library, system threading, and libpqxx
target_link_libraries(APIServer
    PRIVATE Crow::Crow
    PRIVATE pthread
    PRIVATE ${libpqxx_LIBRARIES}
)
