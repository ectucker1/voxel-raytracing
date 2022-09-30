#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "engine/resource.hpp"

class Engine;

class Texture2D : public AResource
{
public:
    uint32_t width, height;
    uint32_t channels;

    vk::Image image;
    vk::ImageView imageView;
    vk::Sampler sampler;

private:
    VmaAllocation allocation;

public:
    Texture2D(const std::shared_ptr<Engine>& engine,
              const std::string& filepath,
              int desiredChannels, vk::Format imageFormat);
};
