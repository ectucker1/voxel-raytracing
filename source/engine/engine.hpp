#pragma once

#include <memory>
#include <functional>
#include <vulkan/vulkan.hpp>
#pragma warning(push, 0)
#include <vk_mem_alloc.h>
#pragma warning(pop)
#include <glm/glm.hpp>
#include "engine/swapchain.hpp"
#include "util/deletion_queue.hpp"
#include "util/resource_ring.hpp"
#include "util/bidirectional_event_queue.hpp"

struct GLFWwindow;
class ARenderer;

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
    vk::Device device;
    PFN_vkGetInstanceProcAddr getInstanceProcAddr;
    PFN_vkGetDeviceProcAddr getDeviceProcAddr;

    VmaAllocator allocator;

    vk::SurfaceKHR surface;

    DeletionQueue deletionQueue;

    Swapchain swapchain;

    vk::Queue graphicsQueue;
    uint32_t graphicsQueueFamily;

    vk::CommandPool renderCommandPool;
    ResourceRing<vk::CommandBuffer> renderCommandBuffers;

    vk::CommandPool uploadCommandPool;
    vk::CommandBuffer uploadCommandBuffer;

    vk::DescriptorPool descriptorPool;

    ResourceRing<vk::Semaphore> presentSemaphores;
    ResourceRing<vk::Semaphore> renderSemaphores;
    ResourceRing<vk::Fence> renderFences;

    std::shared_ptr<ARenderer> renderer;

private:
    bool _initialized;
    uint32_t _frameCount;

public:
    void init();
    void setRenderer(const std::shared_ptr<ARenderer>& renderer);
    void run();
    void destroy();

    void upload_submit(const std::function<void(const vk::CommandBuffer& cmd)>& recordCommands);

private:
    void draw(float delta);

    void resize();

    void initGLFW();
    void initVulkan();
    void initSyncStructures();
};
