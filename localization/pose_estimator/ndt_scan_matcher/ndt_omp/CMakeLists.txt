cmake_minimum_required(VERSION 3.9)
project(ndt_omp)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-Wno-unused-parameter)
endif()

set(CMAKE_BUILD_TYPE "Release")

find_package(ament_cmake REQUIRED)
find_package(PCL REQUIRED COMPONENTS common filters kdtree registration io)
find_package(OpenMP)

###########
## Build ##
###########

add_library(ndt_omp
  src/ndt_omp/voxel_grid_covariance_omp.cpp
  src/ndt_omp/ndt_omp.cpp
)

target_include_directories(ndt_omp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> 
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(ndt_omp
    PRIVATE
      # -mavx causes a lot of errors!!
      -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2
  )
endif()

set(NDT_OMP_DEPENDENCIES
  PCL
  OpenMP
)
# See https://cliutils.gitlab.io/modern-cmake/chapters/packages/OpenMP.html
if(OpenMP_CXX_FOUND)
  target_link_libraries(ndt_omp PRIVATE OpenMP::OpenMP_CXX)
else()
  message(WARNING "OpenMP not found")
endif()

# ament_target_dependencies(ndt_omp PUBLIC OpenMP)
target_compile_definitions(ndt_omp PUBLIC ${PCL_DEFINITIONS})
target_include_directories(ndt_omp PUBLIC ${PCL_INCLUDE_DIRS})
target_link_libraries(ndt_omp PUBLIC ${PCL_LIBRARIES})
target_link_directories(ndt_omp PUBLIC ${PCL_LIBRARY_DIRS})

ament_export_targets(export_ndt_omp HAS_LIBRARY_TARGET)
ament_export_dependencies(${NDT_OMP_DEPENDENCIES})

add_executable(align
  apps/align.cpp
)

# Naively, I'd want to do something like:
# ament_target_dependencies(align PUBLIC PCL OpenMP ndt_omp)
# But there are problems with that:
# One, you can't depend on a local library target: "ament_target_dependencies()
# the passed package name 'ndt_omp' was not found" – so we need to add ndt_omp
# with `target_link_libraries(align PUBLIC ndt_omp)` instead.
target_link_libraries(align PUBLIC ndt_omp)
# Problem two: Now you have a hundred errors like "undefined reference to
# `pcl::something::Something`". That's because it only links
# `libpcl_common.so` for whatever reason, but we also want `libpcl_io.so` etc.
# So we do this PCL thing manually:
target_compile_definitions(align PUBLIC ${PCL_DEFINITIONS})
target_include_directories(align PUBLIC ${PCL_INCLUDE_DIRS})
target_link_libraries(align PUBLIC ${PCL_LIBRARIES})
target_link_directories(align PUBLIC ${PCL_LIBRARY_DIRS})
# That links in the components from `find_package()` above, so that's good. Now
# we're left with `ament_target_dependencies(align PUBLIC OpenMP)`
# ld complains "cannot find -lOpenMP"
ament_target_dependencies(align PUBLIC OpenMP)
#target_link_libraries(align PUBLIC OpenMP)
# ament_target_dependencies(align PUBLIC OpenMP)
# Also link ndt_omp separately because ament_target_dependencies doesn't support
# depending on a library target from the same package


install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS ndt_omp
  EXPORT export_ndt_omp
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

ament_package()

