#pragma once

#include <memory>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "engine/swapchain.hpp"
#include "util/deletion_queue.hpp"
#include "engine/pipeline_storage.hpp"
#include "util/resource_ring.hpp"
#include "util/bidirectional_event_queue.hpp"

class GLFWwindow;

#define MAX_FRAMES_IN_FLIGHT 2

// A wrapper that creates all the vulkan objects that other components can depend on.
class Engine
    : public std::enable_shared_from_this<Engine>
{
public:
    GLFWwindow* window;
    glm::uvec2 windowSize = { 1280, 720 };
    bool windowResized = false;

    BidirectionalEventQueue resizeListeners;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;

    vk::PhysicalDevice physicalDevice;
    vk::Device logicalDevice;

    vk::SurfaceKHR surface;

    DeletionQueue mainDeletionQueue;

    Swapchain swapchain;

    vk::Queue graphicsQueue;
    uint32_t graphicsQueueFamily;

    vk::CommandPool commandPool;
    ResourceRing<vk::CommandBuffer> commandBuffers;

    ResourceRing<vk::Semaphore> presentSemaphores;
    ResourceRing<vk::Semaphore> renderSemaphores;
    ResourceRing<vk::Fence> renderFences;

    vk::RenderPass renderPass;

    ResourceRing<vk::Framebuffer> framebuffers;

    PipelineStorage graphicsPipeline;

private:
    bool _initialized;
    uint32_t _frameCount;

public:
    void init(const std::function<PipelineStorage()>& buildPipeline);
    void run();
    void destroy();

private:
    void draw();

    void resize();

    void initGLFW();
    void initVulkan();
    void initSyncStructures();
    void initDefaultRenderpass();
    void initFramebuffers();
};
