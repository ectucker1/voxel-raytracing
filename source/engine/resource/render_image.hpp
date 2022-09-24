#pragma once

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

class Engine;

class RenderImage
{
public:
    uint32_t width, height = 0;

    vk::Image image;
    vk::Format format;
    vk::ImageView imageView;
    vk::Sampler sampler;

private:
    VmaAllocation allocation;

public:
    RenderImage(const std::shared_ptr<Engine>& engine, uint32_t width, uint32_t height,
                vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);
};
