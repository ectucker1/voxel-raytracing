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
    images = std::vector<vk::Image>(resultImages.size());
    for (int i = 0; i < resultImages.size(); i++)
    {
        images[i] = resultImages[i];
    }
    auto resultImageViews = vkbSwapchain.get_image_views().value();
    imageViews = std::vector<vk::ImageView>(resultImageViews.size());
    for (int i = 0; i < resultImageViews.size(); i++)
    {
        imageViews[i] = resultImageViews[i];
    }

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    renderFence = _engine->logicalDevice.createFence(fenceInfo);

    vk::SemaphoreCreateInfo semaphoreInfo;
    presentSemaphore = _engine->logicalDevice.createSemaphore(semaphoreInfo);
    renderSemaphore = _engine->logicalDevice.createSemaphore(semaphoreInfo);

    _deletorGroup = _engine->mainDeletionQueue.push_group([&]() {
        _engine->logicalDevice.destroySwapchainKHR(swapchain);

        for (int i = 0; i < imageViews.size(); i++)
        {
            _engine->logicalDevice.destroyImageView(imageViews[i]);
        }

        _engine->logicalDevice.destroy(renderFence);

        _engine->logicalDevice.destroy(presentSemaphore);
        _engine->logicalDevice.destroy(renderSemaphore);
    });
}

void Swapchain::initFramebuffers(const vk::RenderPass& renderPass)
{
    _fbRenderPass = renderPass;

    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = _fbRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = _engine->windowSize.x;
    framebufferInfo.height = _engine->windowSize.y;
    framebufferInfo.layers = 1;

    size_t imageCount = images.size();
    framebuffers = std::vector<vk::Framebuffer>(imageCount);

    for (size_t i = 0; i < imageCount; i++)
    {
        framebufferInfo.pAttachments = &imageViews[i];
        framebuffers[i] = _engine->logicalDevice.createFramebuffer(framebufferInfo);
    }

    _engine->mainDeletionQueue.push_deletor(_deletorGroup, [&]() {
        for (int i = 0; i < framebuffers.size(); i++)
        {
            _engine->logicalDevice.destroyFramebuffer(framebuffers[i]);
        }
    });
}

void Swapchain::recreate()
{
    // Wait until window is not minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(_engine->window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(_engine->window, &width, &height);
        glfwWaitEvents();
    }
    _engine->windowSize = { width, height };
    _engine->windowResized = false;

    _engine->logicalDevice.waitIdle();

    _engine->mainDeletionQueue.destroy_group(_deletorGroup);

    init(_engine);
    initFramebuffers(_fbRenderPass);
}
