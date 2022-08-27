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

    vk::Queue graphicsQueue;
    uint32_t graphicsQueueFamily;

    vk::CommandPool commandPool;
    vk::CommandBuffer mainCommandBuffer;

    vk::RenderPass renderPass;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore presentSemaphore;
    vk::Semaphore renderSemaphore;
    vk::Fence renderFence;

private:
    bool _initialized;
    uint32_t _frameCount;

public:
    void init();
    void run();
    void destroy();

private:
    void draw();

    void initGLFW();
    void initVulkan();
    void initCommands();
    void initDefaultRenderpass();
    void initFramebuffers();
    void initSyncStructures();
};
