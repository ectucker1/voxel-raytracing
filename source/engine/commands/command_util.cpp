#include "command_util.hpp"

void cmdutil::imageMemoryBarrier(vk::CommandBuffer commandBuffer, vk::Image image,
                                 vk::AccessFlagBits srcAccess, vk::AccessFlagBits dstAccess,
                                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                 vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage,
                                 vk::ImageAspectFlagBits aspect)
{
    vk::ImageMemoryBarrier presentBarrier = {};
    presentBarrier.srcAccessMask = srcAccess;
    presentBarrier.dstAccessMask = dstAccess;
    presentBarrier.oldLayout = oldLayout;
    presentBarrier.newLayout = newLayout;
    presentBarrier.subresourceRange.aspectMask = aspect;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;
    presentBarrier.image = image;
    commandBuffer.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(0),
                                  0, nullptr, 0, nullptr, 1, &presentBarrier);

}

void cmdutil::blit(vk::CommandBuffer commandBuffer,
                   vk::Image src, glm::ivec2 srcOffset, glm::ivec2 srcSize,
                   vk::Image dst, glm::ivec2 dstOffset, glm::ivec2 dstSize)
{
    // Transition destination image to transfer destination layout
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            dst,
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::ImageAspectFlagBits::eColor);

    // Transition source image from undefined to transfer source layout
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            src,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::ImageAspectFlagBits::eColor);

    vk::ImageBlit blitInfo = {};
    blitInfo.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitInfo.srcSubresource.mipLevel = 0;
    blitInfo.srcSubresource.baseArrayLayer = 0;
    blitInfo.srcSubresource.layerCount = 1;
    blitInfo.srcOffsets[0].x = srcOffset.x;
    blitInfo.srcOffsets[0].y = srcOffset.y;
    blitInfo.srcOffsets[0].z = 0;
    blitInfo.srcOffsets[1].x = srcSize.x;
    blitInfo.srcOffsets[1].y = srcSize.y;
    blitInfo.srcOffsets[1].z = 1;
    blitInfo.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitInfo.dstSubresource.mipLevel = 0;
    blitInfo.dstSubresource.baseArrayLayer = 0;
    blitInfo.dstSubresource.layerCount = 1;
    blitInfo.dstOffsets[0].x = dstOffset.x;
    blitInfo.dstOffsets[0].y = dstOffset.y;
    blitInfo.dstOffsets[0].z = 0;
    blitInfo.dstOffsets[1].x = dstSize.x;
    blitInfo.dstOffsets[1].y = dstSize.y;
    blitInfo.dstOffsets[1].z = 1;
    commandBuffer.blitImage(src, vk::ImageLayout::eTransferSrcOptimal,
                            dst, vk::ImageLayout::eTransferDstOptimal,
                            1, &blitInfo, vk::Filter::eLinear);
}
