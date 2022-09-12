#include "swapchain.hpp"

#include <VkBootstrap.h>
#include <fmt/format.h>
#include "engine/engine.hpp"
#include <glfw/glfw3.h>

void Swapchain::init(const std::shared_ptr<Engine>& engine)
{
    _engine = engine;

    vkb::SwapchainBuilder swapchainBuilder(_engine->physicalDevice, engine->logicalDevice, engine->surface);
    auto swapchainResult = swapchainBuilder
            .use_default_format_selection()
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
    images = ResourceRing<vk::Image>(resultImages.size());
    images.create(resultImages.size(), [&](size_t i) {
        return resultImages[i];
    });

    auto resultImageViews = vkbSwapchain.get_image_views().value();
    imageViews = ResourceRing<vk::ImageView>(resultImageViews.size());
    imageViews.create(resultImageViews.size(), [&](size_t i) {
        return resultImageViews[i];
    });

    uint32_t swapchainGroup = _engine->mainDeletionQueue.push_group([&]() {
        _engine->logicalDevice.destroySwapchainKHR(swapchain);

        imageViews.destroy([&](const vk::ImageView& view) {
            _engine->logicalDevice.destroyImageView(view);
        });
    });

    engine->resizeListeners.push([=]() { _engine->mainDeletionQueue.destroy_group(swapchainGroup); },
                                 [=]() { init(_engine); });
}
