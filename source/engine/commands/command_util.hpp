#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace cmdutil
{
    void imageMemoryBarrier(vk::CommandBuffer commandBuffer, vk::Image image,
                            vk::AccessFlagBits srcAccess, vk::AccessFlagBits dstAccess,
                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                            vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                            vk::ImageAspectFlagBits aspect);

    void blit(vk::CommandBuffer commandBuffer,
              vk::Image src, glm::ivec2 srcOffset, glm::ivec2 srcSize,
              vk::Image dst, glm::ivec2 dstOffset, glm::ivec2 dstSize);
}
