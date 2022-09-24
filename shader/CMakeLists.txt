# Shader compilation to build folder
# Inspired by https://stackoverflow.com/questions/57478571/why-wont-cmake-build-my-vulkan-spirv-shaders

# Get shader directories
set(SHADER_SOURCE_DIR .)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Find GLSLC
find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

# Glob for shader files
file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
    ${SHADER_SOURCE_DIR}/*.vert
    ${SHADER_SOURCE_DIR}/*.frag
    ${SHADER_SOURCE_DIR}/*.comp
    ${SHADER_SOURCE_DIR}/*.geom
    ${SHADER_SOURCE_DIR}/*.tesc
    ${SHADER_SOURCE_DIR}/*.tese
    ${SHADER_SOURCE_DIR}/*.mesh
    ${SHADER_SOURCE_DIR}/*.task
    ${SHADER_SOURCE_DIR}/*.rgen
    ${SHADER_SOURCE_DIR}/*.rchit
    ${SHADER_SOURCE_DIR}/*.rmiss
)

# Create shader binary dir
add_custom_command(
    COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
    OUTPUT ${SHADER_BINARY_DIR}
    COMMENT "Creating ${SHADER_BINARY_DIR}"
)

# Compile each shader in the list
cmake_policy(SET CMP0116 NEW) # CMake policy 0116 - use paths relative to CMAKE_CURRENT_BINARY_DIR for Ninja depfiles
foreach(source IN LISTS SHADER_SOURCES)
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
        COMMAND
            ${glslc_executable}
            -MD -MF ${FILENAME}.d
            -o ${SHADER_BINARY_DIR}/${FILENAME}.spv ${source}
        OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
        DEPENDS ${FILENAME} ${SHADER_BINARY_DIR}
        DEPFILE ${FILENAME}.d
        COMMENT "Building shader ${FILENAME}.spv"
    )
    list(APPEND SHADER_SPV ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

# Add shader target depending on all shaders
add_custom_target(voxels_shader ALL DEPENDS ${SHADER_SPV})
