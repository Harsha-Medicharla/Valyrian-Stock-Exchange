# Install script for directory: /Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/libhiredis.1.1.0.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhiredis.1.1.0.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhiredis.1.1.0.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhiredis.1.1.0.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/libhiredis.dylib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/build/native" TYPE FILE FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/hiredis.targets")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/hiredis" TYPE FILE FILES
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/hiredis.h"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/read.h"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/sds.h"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/async.h"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/alloc.h"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/sockcompat.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/hiredis" TYPE DIRECTORY FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-src/adapters")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/hiredis.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis/hiredis-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis/hiredis-targets.cmake"
         "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/CMakeFiles/Export/f76fe032f6b17c351203f51eb0d9a53a/hiredis-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis/hiredis-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis/hiredis-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis" TYPE FILE FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/CMakeFiles/Export/f76fe032f6b17c351203f51eb0d9a53a/hiredis-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis" TYPE FILE FILES "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/CMakeFiles/Export/f76fe032f6b17c351203f51eb0d9a53a/hiredis-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/hiredis" TYPE FILE FILES
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/hiredis-config.cmake"
    "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/hiredis-config-version.cmake"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/hiredis_fc-build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
