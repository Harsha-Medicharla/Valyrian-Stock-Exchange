add_library(server INTERFACE)
target_include_directories(server INTERFACE include src)
target_link_libraries(server INTERFACE ems)
