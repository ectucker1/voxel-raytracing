#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

class Engine;

class Buffer {
public:
    vk::Buffer buffer;
    VmaAllocation allocation;
public:
    void init(const std::shared_ptr<Engine>& engine, size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage);
};
