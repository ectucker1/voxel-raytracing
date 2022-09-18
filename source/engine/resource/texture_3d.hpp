#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

class Engine;

class Texture3D
{
public:
    uint32_t width, height, depth = 0;

    vk::Image image;
    vk::ImageView imageView;
    vk::Sampler sampler;

private:
    VmaAllocation allocation;
    bool _loaded = false;
public:
    void init(const std::shared_ptr<Engine>& engine,
              void* imageData,
              uint32_t width, uint32_t height, uint32_t depth,
              uint32_t pixelSize, vk::Format imageFormat);
};
