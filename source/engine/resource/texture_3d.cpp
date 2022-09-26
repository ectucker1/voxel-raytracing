#include "texture_3d.hpp"

#include "engine/engine.hpp"
#include "engine/resource/buffer.hpp"

Texture3D::Texture3D(const std::shared_ptr<Engine>& engine,
                     void* imageData,
                     size_t width, size_t height, size_t depth,
                     size_t pixelSize, vk::Format imageFormat)
                     : AResource(engine), width(width), height(height), depth(depth)
{
    vk::DeviceSize imageSize = width * height * depth * pixelSize;

    // Create CPU-side buffer to hold data
    Buffer stagingBuffer(engine, imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy data to buffer
    stagingBuffer.copyData(imageData, static_cast<size_t>(imageSize));

    // Extents
    vk::Extent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(width);
    imageExtent.height = static_cast<uint32_t>(height);
    imageExtent.depth = static_cast<uint32_t>(depth);

    // Image create info
    vk::ImageCreateInfo imageInfo = {};
    imageInfo.imageType = vk::ImageType::e3D;
    imageInfo.extent = imageExtent;
    imageInfo.format = imageFormat;
    imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;

    // Allocation info
    VmaAllocationCreateInfo imageAllocInfo = {};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // Actually create the image
    VkImageCreateInfo imageInfoC = VkImageCreateInfo(imageInfo);
    VkImage imageC;
    auto res = vmaCreateImage(engine->allocator, &imageInfoC, &imageAllocInfo, &imageC, &allocation, nullptr);
    vk::resultCheck(vk::Result(res), "Error creating image");
    image = vk::Image(imageC);

    // Submit commands to copy and transition image
    engine->upload_submit([&](vk::CommandBuffer cmd) {
        // The range to transfer, assumed to be color
        vk::ImageSubresourceRange range = {};
        range.aspectMask = vk::ImageAspectFlagBits::eColor;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        // Barrier to get into transfer destination layout
        vk::ImageMemoryBarrier imageBarrierTransfer = {};
        imageBarrierTransfer.oldLayout = vk::ImageLayout::eUndefined;
        imageBarrierTransfer.newLayout = vk::ImageLayout::eTransferDstOptimal;
        imageBarrierTransfer.image = image;
        imageBarrierTransfer.subresourceRange = range;
        imageBarrierTransfer.srcAccessMask = vk::AccessFlagBits::eNone;
        imageBarrierTransfer.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                            vk::DependencyFlags(0), 0, nullptr, 0, nullptr,
                            1, &imageBarrierTransfer);

        // Copy staging buffer to image
        vk::BufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;
        cmd.copyBufferToImage(stagingBuffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

        // Barrier to get into shader read layout
        vk::ImageMemoryBarrier imageBarrierShader = {};
        imageBarrierShader.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imageBarrierShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageBarrierShader.image = image;
        imageBarrierShader.subresourceRange = range;
        imageBarrierShader.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imageBarrierShader.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                            vk::DependencyFlags(0), 0, nullptr, 0, nullptr,
                            1, &imageBarrierShader);
    });

    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        vmaDestroyImage(engine->allocator, image, allocation);
    });
    stagingBuffer.destroy();

    // Create image view
    vk::ImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.viewType = vk::ImageViewType::e3D;
    imageViewInfo.image = image;
    imageViewInfo.format = imageFormat;
    imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageView = engine->device.createImageView(imageViewInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        engine->device.destroy(imageView);
    });

    // Create sampler
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    sampler = engine->device.createSampler(samplerInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        engine->device.destroy(sampler);
    });
}
