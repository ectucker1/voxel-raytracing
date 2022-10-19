find_package(imgui CONFIG)
find_package(glfw3 CONFIG)
find_package(Vulkan REQUIRED)

set(IMGUI_SOURCES ${IMGUI_SOURCES}
    backends/imgui_impl_glfw.h
    backends/imgui_impl_vulkan.h
    misc/cpp/imgui_stdlib.h
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_vulkan.cpp
    misc/cpp/imgui_stdlib.cpp
)
add_library(imgui_impl_misc STATIC ${IMGUI_SOURCES})
target_link_libraries(imgui_impl_misc imgui::imgui glfw Vulkan::Headers)

add_library(imgui_extra INTERFACE)
target_include_directories(imgui_extra INTERFACE backends misc)
target_link_libraries(imgui_extra INTERFACE imgui_impl_misc)
