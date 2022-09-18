#include "texture_3d.hpp"

#include "engine/engine.hpp"
#include "engine/resource/buffer.hpp"

void Texture3D::init(const std::shared_ptr<Engine>& engine,
                     void* imageData,
                     uint32_t imageWidth, uint32_t imageHeight, uint32_t imageDepth,
                     uint32_t pixelSize, vk::Format imageFormat)
{
    width = imageWidth;
    height = imageHeight;
    depth = imageDepth;
    vk::DeviceSize imageSize = width * height * depth * pixelSize;

    // Create CPU-side buffer to hold data
    Buffer stagingBuffer;
    stagingBuffer.init(engine, imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    // Copy data to buffer
    void* data;
    vmaMapMemory(engine->allocator, stagingBuffer.allocation, &data);
    std::memcpy(data, imageData, static_cast<size_t>(imageSize));
    vmaUnmapMemory(engine->allocator, stagingBuffer.allocation);

    // Extents
    vk::Extent3D imageExtent;
    imageExtent.width = width;
    imageExtent.height = height;
    imageExtent.depth = depth;

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

    engine->mainDeletionQueue.push_group([=]() {
        vmaDestroyImage(engine->allocator, image, allocation);
    });
    vmaDestroyBuffer(engine->allocator, stagingBuffer.buffer, stagingBuffer.allocation);

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
    imageView = engine->logicalDevice.createImageView(imageViewInfo);
    engine->mainDeletionQueue.push_group([=]() {
        engine->logicalDevice.destroy(imageView);
    });

    // Create sampler
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    sampler = engine->logicalDevice.createSampler(samplerInfo);
    engine->mainDeletionQueue.push_group([=]() {
        engine->logicalDevice.destroy(sampler);
    });

    _loaded = true;
}
