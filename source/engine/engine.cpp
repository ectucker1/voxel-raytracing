#include "engine.hpp"

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <fmt/format.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Engine::init(const std::function<PipelineStorage()>& buildPipeline) {
    initGLFW();
    initVulkan();
    initCommands();
    initSyncStructures();
    initDefaultRenderpass();
    initFramebuffers();
    graphicsPipeline = buildPipeline();
    mainDeletionQueue.push_group([&]() {
        logicalDevice.destroy(graphicsPipeline.layout);
        logicalDevice.destroy(graphicsPipeline.pipeline);
    });

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

    const vk::Fence& renderFence = renderFences.next();
    const vk::Semaphore& presentSemaphore = presentSemaphores.next();
    const vk::Semaphore& renderSemaphore = renderSemaphores.next();
    const vk::CommandBuffer& commandBuffer = commandBuffers.next();

    // Wait for GPU to finish work
    res = logicalDevice.waitForFences(1, &renderFence, true, 1000000000);
    vk::resultCheck(res, "Error waiting for fences");

    // Request image from swapchain
    vk::ResultValue<uint32_t> imageIndexResult = logicalDevice.acquireNextImageKHR(swapchain.swapchain, 1000000000, presentSemaphore);
    if (imageIndexResult.result == vk::Result::eErrorOutOfDateKHR || windowResized)
    {
        resize();
        return;
    }
    else if (imageIndexResult.result != vk::Result::eSuccess && imageIndexResult.result != vk::Result::eSuboptimalKHR)
    {
        vk::throwResultException(imageIndexResult.result, "Failed to acquire swapchain image");
    }
    uint32_t imageIndex = imageIndexResult.value;

    // Reset fences
    res = logicalDevice.resetFences(1, &renderFence);
    vk::resultCheck(res, "Error resetting fences");

    // Reset command buffer
    commandBuffer.reset();

    // Begin command buffer
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(cmdBeginInfo);

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
    commandBuffer.beginRenderPass(renderpassInfo, vk::SubpassContents::eInline);

    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.pipeline);

    // Set viewport
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(windowSize.x);
    viewport.height = static_cast<float>(windowSize.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // Set scissor
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(windowSize.x, windowSize.y);
    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.draw(3, 1, 0, 0);

    // End main renderpass
    commandBuffer.endRenderPass();

    // End command buffer
    commandBuffer.end();

    // Submit command buffer to queue
    vk::SubmitInfo submitInfo;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
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

void Engine::resize()
{
    // Wait until window is not minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    windowSize = { width, height };
    windowResized = false;

    logicalDevice.waitIdle();

    resizeListeners.fireListeners();

    logicalDevice.waitIdle();
}

void Engine::destroy() {
    logicalDevice.waitIdle();
    mainDeletionQueue.destroy_all();

    _initialized = false;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->windowSize = { width, height };
    engine->windowResized = true;
}

void Engine::initGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowSize.x, windowSize.y, "Voxel Engine", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    mainDeletionQueue.push_group([&]() {
        glfwDestroyWindow(window);
        glfwTerminate();
    });
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
    mainDeletionQueue.push_group([&]() {
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        instance.destroy();
    });

    // Create GLFW Vulkan surface
    VkSurfaceKHR initSurface;
    VkResult err = glfwCreateWindowSurface(instance, window, nullptr, &initSurface);
    if (err != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface.");
    }
    surface = initSurface;
    mainDeletionQueue.push_group([&]() {
        instance.destroySurfaceKHR(surface);
    });

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
    mainDeletionQueue.push_group([&]() {
        logicalDevice.destroy();
    });

    // Get queues from device
    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Create swapchain
    swapchain.init(shared_from_this());
}

void Engine::initCommands() {
    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamily);
    commandPool = logicalDevice.createCommandPool(commandPoolInfo);
    mainDeletionQueue.push_group([&]() {
        logicalDevice.destroy(commandPool);
    });

    vk::CommandBufferAllocateInfo commandBufferInfo(commandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT);
    auto commandBufferResult = logicalDevice.allocateCommandBuffers(commandBufferInfo);
    commandBuffers.create(MAX_FRAMES_IN_FLIGHT, [&](size_t i) {
        return commandBufferResult[i];
    });
}

void Engine::initSyncStructures() {
    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    renderFences.create(MAX_FRAMES_IN_FLIGHT, [&](size_t) {
        return logicalDevice.createFence(fenceInfo);
    });

    vk::SemaphoreCreateInfo semaphoreInfo;
    presentSemaphores.create(MAX_FRAMES_IN_FLIGHT, [&](size_t) {
        return logicalDevice.createSemaphore(semaphoreInfo);
    });
    renderSemaphores.create(MAX_FRAMES_IN_FLIGHT, [&](size_t) {
        return logicalDevice.createSemaphore(semaphoreInfo);
    });

    uint32_t syncGroup = mainDeletionQueue.push_group([&]() {
        renderFences.destroy([&](const vk::Fence& fence) {
            logicalDevice.destroy(fence);
        });
        presentSemaphores.destroy([&](const vk::Semaphore& semaphore) {
            logicalDevice.destroy(semaphore);
        });
        renderSemaphores.destroy([&](const vk::Semaphore& semaphore) {
            logicalDevice.destroy(semaphore);
        });
    });

    resizeListeners.push([=]() { mainDeletionQueue.destroy_group(syncGroup); },
                         [=]() { initSyncStructures(); });
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
    mainDeletionQueue.push_group([&]() {
        logicalDevice.destroyRenderPass(renderPass);
    });
}

void Engine::initFramebuffers()
{
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = windowSize.x;
    framebufferInfo.height = windowSize.y;
    framebufferInfo.layers = 1;

    size_t imageCount = swapchain.imageViews.size();
    framebuffers = ResourceRing<vk::Framebuffer>(imageCount);
    framebuffers.create(imageCount, [&](size_t i) {
        framebufferInfo.pAttachments = &swapchain.imageViews[i];
        return logicalDevice.createFramebuffer(framebufferInfo);
    });

    uint32_t framebufferGroup = mainDeletionQueue.push_group([&]() {
        framebuffers.destroy([&](const vk::Framebuffer& framebuffer) {
            logicalDevice.destroy(framebuffer);
        });
    });

    resizeListeners.push([=]() { mainDeletionQueue.destroy_group(framebufferGroup); },
                         [=]() { initFramebuffers(); });
}
