cmake_minimum_required(VERSION 3.5)
project(ndt_scan_matcher)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_sensor_msgs REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(ndt_pcl_modified REQUIRED)
find_package(ndt_omp REQUIRED)
find_package(ndt REQUIRED)
find_package(autoware_localization_srvs REQUIRED)
find_package(PCL REQUIRED COMPONENTS registration)

add_executable(ndt_scan_matcher
  src/ndt_scan_matcher_node.cpp
  src/ndt_scan_matcher_core.cpp
)

target_include_directories(ndt_scan_matcher
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set(NDT_SCAN_MATCHER_DEPENDENCIES
  rclcpp
  tf2
  tf2_ros
  tf2_sensor_msgs
  tf2_geometry_msgs
  std_msgs
  geometry_msgs
  sensor_msgs
  nav_msgs
  visualization_msgs
  diagnostic_msgs
  pcl_conversions
  ndt_pcl_modified
  ndt_omp
  ndt
  autoware_localization_srvs
)

ament_target_dependencies(ndt_scan_matcher ${NDT_SCAN_MATCHER_DEPENDENCIES})
ament_export_dependencies(${NDT_SCAN_MATCHER_DEPENDENCIES})

install(
  TARGETS ndt_scan_matcher
  EXPORT export_ndt_scan_matcher
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(
  DIRECTORY launch
  DESTINATION share
)

ament_package()
