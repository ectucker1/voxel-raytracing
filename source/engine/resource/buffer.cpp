#include "buffer.hpp"

#include "engine/engine.hpp"
#include "engine/debug_marker.hpp"

Buffer::Buffer(const std::shared_ptr<Engine>& engine,
               size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memoryUsage, const std::string& name)
               : AResource(engine), size(size)
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

    DebugMarker::setObjectName(engine->device, (VkBuffer)buffer, name);

    vk::Buffer localBuffer = buffer;
    VmaAllocation localAllocation = allocation;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        vmaDestroyBuffer(delEngine->allocator, localBuffer, localAllocation);
    });
}

void Buffer::copyData(void* data, size_t length) const
{
    void* bufferData;
    vmaMapMemory(engine->allocator, allocation, &bufferData);
    std::memcpy(bufferData, data, length);
    vmaUnmapMemory(engine->allocator, allocation);
}
