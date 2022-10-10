# Project declaration
cmake_minimum_required(VERSION 3.16)
project(voxels VERSION 0.0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Append modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# Include dependencies from Conan
include(conan)
conan_cmake_configure(
    REQUIRES
        vulkan-headers/1.3.216.0
        vk-bootstrap/0.5
        vulkan-memory-allocator/3.0.1
        glfw/3.3.8
        glm/0.9.9.8
        fmt/8.1.1
        stb/cci.20210910
        catch2/2.13.9
    GENERATORS
        cmake_find_package_multi
)
conan_cmake_autodetect(settings)
conan_cmake_install(PATH_OR_REFERENCE . BUILD missing REMOTE conancenter SETTINGS ${settings})

# Include prebuilt dependencies
add_subdirectory(thirdparty/fsr2)
add_subdirectory(thirdparty/opengametools)

# Include werror utility
include(werror)

# Add subprojects
add_subdirectory(resource)
add_subdirectory(shader)
add_subdirectory(source)
add_subdirectory(run)
add_subdirectory(test)
