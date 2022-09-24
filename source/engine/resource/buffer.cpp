#include "buffer.hpp"

#include "engine/engine.hpp"

Buffer::Buffer(const std::shared_ptr<Engine>& engine,
               size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage)
               : AResource(engine)
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
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        vmaDestroyBuffer(engine->allocator, buffer, allocation);
    });
}

void Buffer::copyData(void* data, size_t size) const
{
    void* bufferData;
    vmaMapMemory(engine->allocator, allocation, &bufferData);
    std::memcpy(bufferData, data, size);
    vmaUnmapMemory(engine->allocator, allocation);
}
