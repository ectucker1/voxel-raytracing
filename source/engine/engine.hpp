#pragma once

#include <memory>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "swapchain.hpp"

class GLFWwindow;

// A wrapper that creates all the vulkan objects that other components can depend on.
class Engine
    : std::enable_shared_from_this<Engine>
{
public:
    GLFWwindow* window;
    glm::uvec2 windowSize = { 1280, 720 };

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

    vk::Pipeline graphicsPipeline;

private:
    bool _initialized;
    uint32_t _frameCount;

public:
    void init(const std::function<vk::Pipeline()>& buildPipeline);
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
