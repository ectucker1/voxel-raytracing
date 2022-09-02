#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

class Engine;

// Vulkan swapchain abstraction.
// Should only be created by the main engine.
class Swapchain
{
private:
    std::shared_ptr<Engine> _engine;

    uint32_t _deletorGroup;

    vk::RenderPass _fbRenderPass;
public:
    vk::SwapchainKHR swapchain;

    vk::Format imageFormat;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore presentSemaphore;
    vk::Semaphore renderSemaphore;
    vk::Fence renderFence;

public:
    void init(const std::shared_ptr<Engine>& engine);
    void initFramebuffers(const vk::RenderPass& renderPass);
    void recreate();
};
