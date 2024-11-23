cmake_minimum_required(VERSION 3.14)

add_library(windows_interface STATIC
  "windows_interface.cpp"
  "windows_interface.h"
)

# link Flutter library
target_link_libraries(windows_interface PRIVATE flutter flutter_wrapper_plugin)

# set include directories
target_include_directories(windows_interface PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
