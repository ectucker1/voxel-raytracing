#include "swapchain.hpp"

#include <VkBootstrap.h>
#include <fmt/format.h>

void Swapchain::init(const vk::PhysicalDevice& physicalDevice, const vk::Device& logicalDevice, const vk::SurfaceKHR& surface, uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder(physicalDevice, logicalDevice, surface);
    auto swapchainResult = swapchainBuilder
            .use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(width, height)
            .build();
    if (!swapchainResult) {
        throw std::runtime_error(fmt::format("Failed to create swapchain. Error: {}", swapchainResult.error().message()));
    }
    vkb::Swapchain vkbSwapchain = swapchainResult.value();

    swapchain = vkbSwapchain.swapchain;
    imageFormat = vk::Format(vkbSwapchain.image_format);
    images = vkbSwapchain.get_images().value();
    imageViews = vkbSwapchain.get_image_views().value();
}

void Swapchain::destroy(const vk::Device& logicalDevice) {
    logicalDevice.destroySwapchainKHR(swapchain);

    for (int i = 0; i < imageViews.size(); i++) {
        logicalDevice.destroyImageView(imageViews[i]);
    }
}
