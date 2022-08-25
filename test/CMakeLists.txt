# Unit testing

# Find test package
find_package(Catch2 CONFIG)

# Glob for test source files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add executable test target
add_executable(voxels_test ${TEST_SOURCES})

# Link to main and Catch2 libraries
target_link_libraries(voxels_test voxels_lib Catch2::Catch2)

# Enable werror
target_enable_werror(voxels_test)

# Include CTest adapter
include(CTest)
include(Catch)
catch_discover_tests(voxels_test)
