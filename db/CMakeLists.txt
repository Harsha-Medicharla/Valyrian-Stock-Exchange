cmake_minimum_required(VERSION 3.14)
project(vse_db LANGUAGES CXX)
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

FetchContent_Declare(
  libpq_standalone
  GIT_REPOSITORY https://gitlab.com/sabelka/libpq-standalone.git
  GIT_TAG 7d7badcfc91c95b020da58f15788b8ae0dca0392
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(libpq_standalone)
if(NOT libpq_standalone_POPULATED)
  FetchContent_Populate(libpq_standalone)
  set(LIBPQ_TOPLEVEL_CMAKE "${libpq_standalone_SOURCE_DIR}/CMakeLists.txt")
  if(EXISTS "${LIBPQ_TOPLEVEL_CMAKE}")
    file(READ "${LIBPQ_TOPLEVEL_CMAKE}" _libpq_toplevel_content)
    string(REPLACE "  add_subdirectory(test)\n#endif()\n" "# add_subdirectory(test)\n" _libpq_toplevel_content "${_libpq_toplevel_content}")
    file(WRITE "${LIBPQ_TOPLEVEL_CMAKE}" "${_libpq_toplevel_content}")
  endif()
  add_subdirectory("${libpq_standalone_SOURCE_DIR}" "${libpq_standalone_BINARY_DIR}")
endif()

add_library(vse_db_lib STATIC
  DBWriter.cpp
  PGWriter.cpp
)

target_include_directories(vse_db_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/ems
  ${rigtorp_spscqueue_SOURCE_DIR}/include
)

target_link_libraries(vse_db_lib PUBLIC
  Threads::Threads
  vse_shared
  PostgreSQL::pq
)

add_executable(db_app main.cpp)
target_link_libraries(db_app PRIVATE vse_db_lib)
