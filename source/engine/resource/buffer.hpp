#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "engine/resource.hpp"

class Engine;

class Buffer : public AResource
{
public:
    vk::Buffer buffer;
    size_t size;

private:
    VmaAllocation allocation;

public:
    Buffer::Buffer(const std::shared_ptr<Engine>& engine,
                   size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void copyData(void* data, size_t size) const;
};
