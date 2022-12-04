#include "render_image.hpp"

#include "engine/engine.hpp"
#include "engine/debug_marker.hpp"

RenderImage::RenderImage(const std::shared_ptr<Engine>& engine, uint32_t width, uint32_t height,
                         vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect, const std::string& name)
                         : AResource(engine), width(width), height(height), format(format)
{
    vk::Extent3D imageExtent;
    imageExtent.width = width;
    imageExtent.height = height;
    imageExtent.depth = 1;

    vk::ImageCreateInfo imageInfo = {};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = format;
    imageInfo.extent = imageExtent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = usage;

    // Allocation requirements
    VmaAllocationCreateInfo imageAllocInfo = {};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imageAllocInfo.requiredFlags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // Create image
    VkImageCreateInfo imageInfoC = VkImageCreateInfo(imageInfo);
    VkImage imageC;
    auto res = vmaCreateImage(engine->allocator, &imageInfoC, &imageAllocInfo, &imageC, &allocation, nullptr);
    vk::resultCheck(vk::Result(res), "Error creating render target image");
    image = vk::Image(imageC);
    VmaAllocation localAllocation = allocation;
    DebugMarker::setObjectName(engine->device, (VkImage)image, fmt::format("{} Image", name));
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        vmaDestroyImage(delEngine->allocator, imageC, localAllocation);
    });

    // Create image view
    vk::ImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.image = image;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = aspect;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    vk::ImageView createdImageView = engine->device.createImageView(imageViewInfo);
    imageView = createdImageView;
    DebugMarker::setObjectName(engine->device, (VkImageView)imageView, fmt::format("{} Image View", name));
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdImageView);
    });

    // Create sampler
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    vk::Sampler createdSampler = engine->device.createSampler(samplerInfo);
    sampler = createdSampler;
    DebugMarker::setObjectName(engine->device, (VkSampler)sampler, fmt::format("{} Sampler", name));
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdSampler);
    });
}
