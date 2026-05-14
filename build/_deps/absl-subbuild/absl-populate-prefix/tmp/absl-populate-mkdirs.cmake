# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-src")
  file(MAKE_DIRECTORY "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-src")
endif()
file(MAKE_DIRECTORY
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-build"
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix"
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/tmp"
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp"
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src"
  "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/chandangowdac/Documents/mp[3]/projects/vse/Valyrian_Stock_Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
