#include "swapchain.hpp"

#include <VkBootstrap.h>
#include <fmt/format.h>
#include "engine/engine.hpp"
#include <glfw/glfw3.h>

void Swapchain::init(const std::shared_ptr<Engine>& engine)
{
    _engine = engine;

    vkb::SwapchainBuilder swapchainBuilder(_engine->physicalDevice, engine->device, engine->surface);
    auto swapchainResult = swapchainBuilder
            .use_default_format_selection()
            .add_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(_engine->windowSize.x, _engine->windowSize.y)
            .build();
    if (!swapchainResult)
    {
        throw std::runtime_error(fmt::format("Failed to create swapchain. Error: {}", swapchainResult.error().message()));
    }
    vkb::Swapchain vkbSwapchain = swapchainResult.value();

    swapchain = vkbSwapchain.swapchain;
    imageFormat = vk::Format(vkbSwapchain.image_format);
    auto resultImages = vkbSwapchain.get_images().value();
    images = ResourceRing<vk::Image>::fromFunc(resultImages.size(), [&](size_t i) {
        return resultImages[i];
    });

    auto resultImageViews = vkbSwapchain.get_image_views().value();
    imageViews= ResourceRing<vk::ImageView>::fromFunc(resultImageViews.size(), [&](size_t i) {
        return resultImageViews[i];
    });

    uint32_t swapchainGroup = _engine->deletionQueue.push_group([&]() {
        _engine->device.destroySwapchainKHR(swapchain);

        imageViews.destroy([&](const vk::ImageView& view) {
            _engine->device.destroyImageView(view);
        });
    });

    engine->resizeListeners.push([=]() { _engine->deletionQueue.destroy_group(swapchainGroup); },
                                 [=]() { init(_engine); });
}

size_t Swapchain::size() {
    return images.size();
}
