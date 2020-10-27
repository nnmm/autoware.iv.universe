cmake_minimum_required(VERSION 3.5)
project(surround_obstacle_checker)

### Compile options
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wno-unused-parameter -Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_auto REQUIRED)
find_package(eigen3_cmake_module REQUIRED)
find_package(Eigen3 REQUIRED)

ament_auto_find_build_dependencies()

ament_auto_add_executable(surround_obstacle_checker_node
  src/debug_marker.cpp
  src/node.cpp
  src/main.cpp
)

ament_auto_package(INSTALL_TO_SHARE
  launch config
) 