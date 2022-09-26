#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "engine/resource.hpp"

class Engine;

class Texture3D : public AResource
{
public:
    size_t width, height, depth = 0;

    vk::Image image;
    vk::ImageView imageView;
    vk::Sampler sampler;

private:
    VmaAllocation allocation;

public:
    Texture3D(const std::shared_ptr<Engine>& engine,
              void* imageData,
              size_t width, size_t height, size_t depth,
              size_t pixelSize, vk::Format imageFormat);
};
