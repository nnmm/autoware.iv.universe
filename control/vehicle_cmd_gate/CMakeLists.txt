cmake_minimum_required(VERSION 3.5)
project(vehicle_cmd_gate)

### Compile options
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-Wno-unused-parameter)
endif()

### Dependencies
find_package(ament_cmake REQUIRED)
find_package(autoware_control_msgs REQUIRED)
find_package(autoware_vehicle_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# Add path of include dir
include_directories(include)

# Generate exe file
add_executable(vehicle_cmd_gate src/vehicle_cmd_gate_node.cpp src/vehicle_cmd_gate.cpp src/vehicle_cmd_filter.cpp)

ament_target_dependencies(vehicle_cmd_gate autoware_control_msgs autoware_vehicle_msgs geometry_msgs rclcpp std_msgs)

install(
  TARGETS vehicle_cmd_gate
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY
    launch
    config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
