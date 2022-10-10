# Common library used for both application & tests

# Find packages
find_package(vk-bootstrap CONFIG)
find_package(vulkan-memory-allocator CONFIG)
find_package(glfw3 CONFIG)
find_package(fmt CONFIG)
find_package(glm CONFIG)
find_package(stb CONFIG)
find_package(Vulkan REQUIRED)

# Glob for source files
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

# Add library target out of source files
add_library(voxels_lib STATIC ${LIB_SOURCES})

# Include headers as interface
target_include_directories(voxels_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(voxels_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64")
    set(FSR2_PLATFORM_NAME x64)
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
    set(FSR2_PLATFORM_NAME x86)
endif()

# Link to external libraries
target_link_libraries(voxels_lib
    Vulkan::Headers
    vulkan-memory-allocator::vulkan-memory-allocator
    vk-bootstrap::vk-bootstrap
    glfw
    fmt::fmt
    glm::glm
    stb::stb
    fsr2
    ${Vulkan_LIBRARIES}
    opengametools
)

# Define library include options
target_compile_definitions(voxels_lib PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Enable werror
target_enable_werror(voxels_lib)

# Precompile pch.hpp header
target_precompile_headers(voxels_lib
    PRIVATE
        <memory>
        <vector>
        <unordered_map>
        <unordered_set>
        <deque>
        <functional>
        <optional>
        <fmt/format.h>
        <VkBootstrap.h>
        <vulkan/vulkan.hpp>
        <glm/glm.hpp>
)
