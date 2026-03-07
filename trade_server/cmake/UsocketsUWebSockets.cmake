# Fetches and builds uSockets (C) + header-only uWebSockets (C++). No OpenSSL.

if(TARGET uwebsockets_vse)
  return()
endif()

include(FetchContent)

set(USOCKETS_TAG v0.8.8)
set(UWEBSOCKETS_TAG v20.67.0)

FetchContent_Declare(
  usockets_fc
  GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
  GIT_TAG ${USOCKETS_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(usockets_fc)

FetchContent_Declare(
  uwebsockets_fc
  GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
  GIT_TAG ${UWEBSOCKETS_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(uwebsockets_fc)

FetchContent_Declare(
  zlib_fc
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG 570720b0c24f9686c33f35a1b3165c1f568b96be
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zlib_fc)

set(USOCK_SRC_DIR "${usockets_fc_SOURCE_DIR}/src")

# Paths may contain '[' (e.g. mp[3]); CMake file(GLOB) treats '[' as a glob metacharacter.
function(vse_glob_c out_var glob_dir)
  execute_process(
    COMMAND python3 -c "import glob,sys; from glob import escape; b=sys.argv[1]; print('\\n'.join(glob.glob(escape(b)+'/*.c')))" "${glob_dir}"
    OUTPUT_VARIABLE _lines
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _rc)
  if(NOT _rc EQUAL 0)
    set(_lines "")
  endif()
  string(REGEX REPLACE "\n+$" "" _lines "${_lines}")
  string(REPLACE "\n" ";" _list "${_lines}")
  set(${out_var} "${_list}" PARENT_SCOPE)
endfunction()

vse_glob_c(USOCK_ROOT_C "${USOCK_SRC_DIR}")
vse_glob_c(USOCK_EV_C "${USOCK_SRC_DIR}/eventing")
set(USOCK_SOURCES ${USOCK_ROOT_C} ${USOCK_EV_C})

list(FILTER USOCK_SOURCES EXCLUDE REGEX ".*/openssl\\.c$")
list(FILTER USOCK_SOURCES EXCLUDE REGEX ".*/sni\\.c$")
list(FILTER USOCK_SOURCES EXCLUDE REGEX ".*/wolfssl\\.c$")
list(FILTER USOCK_SOURCES EXCLUDE REGEX ".*/boringssl\\.c$")
list(FILTER USOCK_SOURCES EXCLUDE REGEX ".*io_uring.*")

if(NOT USOCK_SOURCES)
  message(FATAL_ERROR "uSockets sources not found under ${USOCK_SRC_DIR}")
endif()

find_package(Threads REQUIRED)

add_library(usockets_vse STATIC ${USOCK_SOURCES})
target_include_directories(usockets_vse PUBLIC "${USOCK_SRC_DIR}")

target_compile_definitions(usockets_vse PUBLIC LIBUS_NO_SSL)

if(APPLE)
  target_compile_definitions(usockets_vse PUBLIC LIBUS_USE_KQUEUE)
else()
  target_compile_definitions(usockets_vse PUBLIC LIBUS_USE_EPOLL)
endif()

target_link_libraries(usockets_vse PUBLIC Threads::Threads)

target_link_libraries(usockets_vse PUBLIC zlibstatic)

if(UNIX AND NOT APPLE)
  target_link_libraries(usockets_vse PUBLIC z)
elseif(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
  if(COREFOUNDATION_FRAMEWORK)
    target_link_libraries(usockets_vse PUBLIC ${COREFOUNDATION_FRAMEWORK})
  endif()
endif()

add_library(uwebsockets_vse INTERFACE)
target_include_directories(
  uwebsockets_vse INTERFACE
  "${uwebsockets_fc_SOURCE_DIR}/src"
)
target_link_libraries(uwebsockets_vse INTERFACE usockets_vse)
target_compile_definitions(
  uwebsockets_vse
  INTERFACE
    UWS_NO_SSL
)
