# Resource copying to build folder
# Inspired by https://qrikko.blogspot.com/2016/05/cmake-and-how-to-copy-resources-during.html

# Get resource directories
set(RES_SOURCE .)
set(RES_DEST ${CMAKE_CURRENT_BINARY_DIR})

# Glob for resource files
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS
    ${RES_SOURCE}/*.jpg
    ${RES_SOURCE}/*.png
    ${RES_SOURCE}/*.exr
    ${RES_SOURCE}/*.hdr
)

# Create resource folder in output
add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RES_DEST}
    OUTPUT ${RES_DEST}
    COMMENT "Creating ${RES_DEST}"
)

# Copy each resource folder to output
foreach(source IN LISTS RESOURCE_FILES)
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E copy ${source} ${RES_DEST}/${FILENAME}
        OUTPUT ${RES_DEST}/${FILENAME}
        DEPENDS ${source} ${RES_DEST}
        COMMENT "Copying ${FILENAME} to resource folder"
    )
    list(APPEND RESOURCE_OUTPUTS ${RES_DEST}/${FILENAME})
endforeach()

# Add resource target depending on all resources
add_custom_target(voxels_resource ALL DEPENDS ${RESOURCE_OUTPUTS})
