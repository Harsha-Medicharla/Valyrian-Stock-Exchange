# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-src"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-build"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/tmp"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src"
  "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/chris/Valyrian-Stock-Exchange/build/_deps/absl-subbuild/absl-populate-prefix/src/absl-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
