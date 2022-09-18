#include "buffer.hpp"

#include "engine/engine.hpp"

void Buffer::init(const std::shared_ptr<Engine>& engine, size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;

    VkBufferCreateInfo bufferInfoC = VkBufferCreateInfo(bufferInfo);
    VkBuffer outBufferC;
    auto res = vmaCreateBuffer(engine->allocator, &bufferInfoC, &allocInfo, &outBufferC, &allocation, nullptr);
    vk::resultCheck(vk::Result(res), "Error creating buffer");

    buffer = outBufferC;
}
