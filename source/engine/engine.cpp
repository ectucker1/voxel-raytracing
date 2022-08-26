#include "engine.hpp"

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <fmt/format.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define INIT_WINDOW_WIDTH 1280
#define INIT_WINDOW_HEIGHT 720

void Engine::init() {
    initGLFW();
    initVulkan();

    _initialized = true;
}

void Engine::run() {
    if (!_initialized)
        throw std::runtime_error("Cannot run engine before initializing.");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Engine::destroy() {
    if (_initialized) {
        // Must be in reverse order of creation
        swapchain.destroy(logicalDevice);
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

    window = glfwCreateWindow(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, "Voxel Engine", nullptr, nullptr);
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

    // Create swapchain
    swapchain.init(physicalDevice, logicalDevice, surface, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
}
