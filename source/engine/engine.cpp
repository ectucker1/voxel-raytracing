#include "engine.hpp"

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <fmt/format.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Engine::init(const std::function<vk::Pipeline()>& buildPipeline) {
    initGLFW();
    initVulkan();
    initCommands();
    initDefaultRenderpass();
    initFramebuffers();
    initSyncStructures();
    graphicsPipeline = buildPipeline();

    _initialized = true;
}

void Engine::run() {
    if (!_initialized)
        throw std::runtime_error("Cannot run engine before initializing.");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        draw();
    }
}

void Engine::draw() {
    vk::Result res;

    // Wait for GPU to finish work
    res = logicalDevice.waitForFences(1, &renderFence, true, 1000000000);
    vk::resultCheck(res, "Error waiting for fences");
    res = logicalDevice.resetFences(1, &renderFence);
    vk::resultCheck(res, "Error resetting fences");

    // Request image from swapchain
    uint32_t imageIndex = logicalDevice.acquireNextImageKHR(swapchain.swapchain, 1000000000, presentSemaphore).value;

    // Reset command buffer
    mainCommandBuffer.reset();

    // Begin command buffer
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    mainCommandBuffer.begin(cmdBeginInfo);

    // Create clear color for this frame
    vk::ClearValue clearValue;
    float flash = abs(sin(_frameCount / 512.0f));
    clearValue.color = vk::ClearColorValue(std::array<float, 4> {0.0f, 0.0f, flash, 1.0f});

    // Start main renderpass
    vk::RenderPassBeginInfo renderpassInfo;
    renderpassInfo.renderPass = renderPass;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.extent = vk::Extent2D(windowSize.x, windowSize.y);
    renderpassInfo.framebuffer = framebuffers[imageIndex];
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues = &clearValue;
    mainCommandBuffer.beginRenderPass(renderpassInfo, vk::SubpassContents::eInline);

    // Draw
    mainCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    mainCommandBuffer.draw(3, 1, 0, 0);

    // End main renderpass
    mainCommandBuffer.endRenderPass();

    // End command buffer
    mainCommandBuffer.end();

    // Submit command buffer to queue
    vk::SubmitInfo submitInfo;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mainCommandBuffer;
    res = graphicsQueue.submit(1, &submitInfo, renderFence);
    vk::resultCheck(res, "Error submitting command buffer");

    // Present to swapchain
    vk::PresentInfoKHR presentInfo;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &imageIndex;
    res = graphicsQueue.presentKHR(presentInfo);
    vk::resultCheck(res, "Error presenting");

    _frameCount++;
}

void Engine::destroy() {
    if (_initialized) {
        // Must be in reverse order of creation
        logicalDevice.destroy(commandPool);
        swapchain.destroy(logicalDevice);
        logicalDevice.destroyRenderPass(renderPass);
        for (int i = 0; i < framebuffers.size(); i++) {
            logicalDevice.destroyFramebuffer(framebuffers[i]);
        }
        logicalDevice.destroy();
        instance.destroySurfaceKHR(surface);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        instance.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();

        _initialized = false;
    }
}

void Engine::initGLFW() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(windowSize.x, windowSize.y, "Voxel Engine", nullptr, nullptr);
}

void Engine::initVulkan() {
    // Set up dynamic loader
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // Create Vulkan instance
    vkb::InstanceBuilder instanceBuilder;
    auto instanceResult = instanceBuilder
        .set_app_name("Voxel Engine")
        .request_validation_layers(true)
        .require_api_version(1, 1, 0)
        .use_default_debug_messenger()
        .build();
    if (!instanceResult) {
        throw std::runtime_error(fmt::format("Failed to create Vulkan instance. Error: {}", instanceResult.error().message()));
    }
    vkb::Instance vkbInstance = instanceResult.value();
    instance = vkbInstance.instance;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    debugMessenger = vkbInstance.debug_messenger;

    // Create GLFW Vulkan surface
    VkSurfaceKHR initSurface;
    VkResult err = glfwCreateWindowSurface(instance, window, nullptr, &initSurface);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface.");
    }
    surface = initSurface;

    // Select physical device (GPU)
    vkb::PhysicalDeviceSelector physicalDeviceSelector(vkbInstance);
    auto physicalDeviceResult = physicalDeviceSelector
        .set_minimum_version(1, 1)
        .set_surface(surface)
        .select();
    if (!physicalDeviceResult) {
        throw std::runtime_error(fmt::format("Failed to select physical device. Error: {}", physicalDeviceResult.error().message()));
    }
    vkb::PhysicalDevice vkbPhysicalDevice = physicalDeviceResult.value();
    physicalDevice = vkbPhysicalDevice.physical_device;

    // Create logical device
    vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
    auto deviceResult = deviceBuilder.build();
    if (!deviceResult) {
        throw std::runtime_error(fmt::format("Failed to create logical device. Error: {}", physicalDeviceResult.error().message()));
    }
    vkb::Device vkbDevice = deviceResult.value();
    logicalDevice = vkbDevice.device;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(logicalDevice);

    // Get queues from device
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Create swapchain
    swapchain.init(physicalDevice, logicalDevice, surface, windowSize.x, windowSize.y);
}

void Engine::initCommands() {
    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamily);
    commandPool = logicalDevice.createCommandPool(commandPoolInfo);

    vk::CommandBufferAllocateInfo commandBufferInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    mainCommandBuffer = logicalDevice.allocateCommandBuffers(commandBufferInfo).front();
}

void Engine::initDefaultRenderpass() {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapchain.imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    renderPass = logicalDevice.createRenderPass(renderPassInfo);
}

void Engine::initFramebuffers() {
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = windowSize.x;
    framebufferInfo.height = windowSize.y;
    framebufferInfo.layers = 1;

    size_t imageCount = swapchain.images.size();
    framebuffers = std::vector<vk::Framebuffer>(imageCount);

    for (size_t i = 0; i < imageCount; i++) {
        framebufferInfo.pAttachments = &swapchain.imageViews[i];
        framebuffers[i] = logicalDevice.createFramebuffer(framebufferInfo);
    }
}

void Engine::initSyncStructures() {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    renderFence = logicalDevice.createFence(fenceInfo);

    vk::SemaphoreCreateInfo semaphoreInfo;
    presentSemaphore = logicalDevice.createSemaphore(semaphoreInfo);
    renderSemaphore = logicalDevice.createSemaphore(semaphoreInfo);
}
