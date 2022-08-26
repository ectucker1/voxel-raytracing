#pragma once

#include <vulkan/vulkan.hpp>
#include "swapchain.hpp"

class GLFWwindow;

// A wrapper that creates all the vulkan objects that other components can depend on.
class Engine {
public:
    GLFWwindow* window;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;

    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

    vk::SurfaceKHR surface;

    Swapchain swapchain;

private:
    bool _initialized;

public:
    void init();
    void run();
    void destroy();

private:
    void initGLFW();
    void initVulkan();
};
