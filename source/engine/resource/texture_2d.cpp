#include "texture_2d.hpp"

#include "engine/engine.hpp"
#include "engine/resource/buffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fmt/format.h>

static size_t formatSize(vk::Format format)
{
    switch (format)
    {
        case vk::Format::eR32G32B32A32Sfloat:
            return 16;
        case vk::Format::eR8G8B8A8Unorm:
            return 4;
    }
    return 4;
}


Texture2D::Texture2D(const std::shared_ptr<Engine>& engine,
                     const std::string& filepath,
                     int desiredChannels, vk::Format imageFormat)
        : AResource(engine)
{
    int iWidth;
    int iHeight;
    int iChannels;
    void* pixels;
    if (stbi_is_hdr(filepath.c_str()))
    {
        pixels = stbi_loadf(filepath.c_str(), &iWidth, &iHeight, &iChannels, desiredChannels);
    }
    else
    {
        pixels = stbi_load(filepath.c_str(), &iWidth, &iHeight, &iChannels, desiredChannels);
    }

    if (!pixels)
    {
        throw std::runtime_error(fmt::format("Could not load image {}", filepath));
    }

    width = static_cast<uint32_t>(iWidth);
    height = static_cast<uint32_t>(iHeight);
    channels = static_cast<uint32_t>(iChannels);

    vk::DeviceSize imageSize = width * height * formatSize(imageFormat);

    // Create CPU-side buffer to hold data
    Buffer stagingBuffer(engine, imageSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY, "Texture2D Staging Buffer");

    // Copy data to buffer
    stagingBuffer.copyData(pixels, static_cast<size_t>(imageSize));

    stbi_image_free(pixels);

    // Extents
    vk::Extent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(width);
    imageExtent.height = static_cast<uint32_t>(height);
    imageExtent.depth = 1;

    // Image create info
    vk::ImageCreateInfo imageInfo = {};
    imageInfo.imageType = vk::ImageType::e2D;
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

    VmaAllocation localAllocation = allocation;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        vmaDestroyImage(delEngine->allocator, imageC, localAllocation);
    });
    stagingBuffer.destroy();

    // Create image view
    vk::ImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.image = image;
    imageViewInfo.format = imageFormat;
    imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    vk::ImageView createdImageView = engine->device.createImageView(imageViewInfo);
    imageView = createdImageView;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdImageView);
    });

    // Create sampler
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    vk::Sampler createdSampler = engine->device.createSampler(samplerInfo);
    sampler = createdSampler;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdSampler);
    });
}
