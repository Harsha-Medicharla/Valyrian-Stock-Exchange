# CMake generated Testfile for 
# Source directory: /home/chris/Valyrian-Stock-Exchange
# Build directory: /home/chris/Valyrian-Stock-Exchange/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/chris/Valyrian-Stock-Exchange/build/pipeline_integration_test[1]_include.cmake")
include("/home/chris/Valyrian-Stock-Exchange/build/ems_settlement_test[1]_include.cmake")
include("/home/chris/Valyrian-Stock-Exchange/build/matching_engine_test[1]_include.cmake")
subdirs("_deps/absl-build")
subdirs("_deps/googletest-build")
subdirs("MatchingEngine")
subdirs("EMS")
subdirs("Settlement")
