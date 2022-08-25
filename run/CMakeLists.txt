# Main application

# Add executable
add_executable(voxels_run main.cpp)

# Link to main library
target_link_libraries(voxels_run voxels_lib)

# Add resources and shaders as dependencies
add_dependencies(voxels_run voxels_resource voxels_shader)

# Enable werror
target_enable_werror(voxels_run)
