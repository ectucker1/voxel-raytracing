#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

// Vulkan swapchain abstraction.
// Should only be created by the main engine.
class Swapchain {
public:
    vk::SwapchainKHR swapchain;
    vk::Format imageFormat;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

public:
    void init(
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& logicalDevice,
        const vk::SurfaceKHR& surface,
        uint32_t width, uint32_t height);
    void destroy(const vk::Device& logicalDevice);
};
