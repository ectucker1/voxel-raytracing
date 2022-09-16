#include "engine.hpp"

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <fmt/format.h>
#include "demo/screen_quad_push.hpp"
#include "engine/renderer.hpp"
#include <chrono>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void Engine::init(const std::shared_ptr<ARenderer>& _renderer)
{
    initGLFW();
    initVulkan();
    initSyncStructures();

    renderer = _renderer;
    renderer->init(shared_from_this());

    _initialized = true;
}

void Engine::run()
{
    if (!_initialized)
        throw std::runtime_error("Cannot run engine before initializing.");

    using TimePoint = std::chrono::steady_clock::time_point;
    TimePoint start = std::chrono::steady_clock::now();
    TimePoint end = start;
    float delta = 0;

    while (!glfwWindowShouldClose(window))
    {
        start = std::chrono::steady_clock::now();
        glfwPollEvents();
        draw(delta);
        end = std::chrono::steady_clock::now();
        delta = std::chrono::duration<float>(end - start).count();
    }
}

void Engine::draw(float delta) {
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

    // Update logic
    renderer->update(delta);

    // Reset command buffer
    commandBuffer.reset();

    // Begin command buffer
    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(cmdBeginInfo);

    // Record the commands
    renderer->recordCommands(commandBuffer, imageIndex);

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

    // Create command pool
    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamily);
    commandPool = logicalDevice.createCommandPool(commandPoolInfo);
    mainDeletionQueue.push_group([&]() {
        logicalDevice.destroy(commandPool);
    });

    // Create command buffers
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
