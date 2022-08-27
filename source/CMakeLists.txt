# Common library used for both application & tests

# Find packages
find_package(VulkanHeaders CONFIG)
find_package(vk-bootstrap CONFIG)
find_package(glfw3 CONFIG)
find_package(fmt)

# Glob for source files
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add library target out of source files
add_library(voxels_lib STATIC ${LIB_SOURCES})

# Include headers as interface
target_include_directories(voxels_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Link to external libraries
target_link_libraries(voxels_lib
    Vulkan::Headers
    vk-bootstrap::vk-bootstrap
    glfw
    fmt::fmt
)

# Define library include options
target_compile_definitions(voxels_lib PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Enable werror
target_enable_werror(voxels_lib)

# Precompile pch.hpp header
target_precompile_headers(voxels_lib PRIVATE pch.hpp)