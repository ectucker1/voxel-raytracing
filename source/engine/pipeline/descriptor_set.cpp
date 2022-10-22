#include "descriptor_set.hpp"

#include "engine/engine.hpp"

const vk::DescriptorSet* DescriptorSet::getSet(uint32_t frame) const
{
    return &sets[frame];
}

void DescriptorSet::writeBuffer(uint32_t binding, uint32_t frame,
                                vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorType type) const
{
    vk::DescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    vk::WriteDescriptorSet bufferWrite = {};
    bufferWrite.dstBinding = binding;
    bufferWrite.dstSet = sets[frame];
    bufferWrite.descriptorCount = 1;
    bufferWrite.descriptorType = type;
    bufferWrite.pBufferInfo = &bufferInfo;

    engine->device.updateDescriptorSets(1, &bufferWrite, 0, nullptr);
}

void DescriptorSet::writeImage(uint32_t binding, uint32_t frame,
                               vk::ImageView imageView, vk::Sampler sampler, vk::ImageLayout imageLayout) const
{
    vk::DescriptorImageInfo imageInfo {};
    imageInfo.sampler = sampler;
    imageInfo.imageView = imageView;
    imageInfo.imageLayout = imageLayout;

    vk::WriteDescriptorSet imageWrite = {};
    imageWrite.dstBinding = binding;
    imageWrite.dstSet = sets[frame];
    imageWrite.descriptorCount = 1;
    imageWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    imageWrite.pImageInfo = &imageInfo;

    engine->device.updateDescriptorSets(1, &imageWrite, 0, nullptr);
}

void DescriptorSet::initBuffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorType type) const
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        writeBuffer(binding, i, buffer, size, type);
}

void DescriptorSet::initImage(uint32_t binding, vk::ImageView imageView, vk::Sampler sampler, vk::ImageLayout imageLayout) const
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        writeImage(binding, i, imageView, sampler, imageLayout);
}

DescriptorSetBuilder& DescriptorSetBuilder::buffer(uint32_t binding, vk::ShaderStageFlags stages, vk::DescriptorType type)
{
    vk::DescriptorSetLayoutBinding bufferBinding {};
    bufferBinding.descriptorType = type;
    bufferBinding.stageFlags = stages;
    bufferBinding.binding = binding;
    bufferBinding.descriptorCount = 1;
    bindings.push_back(bufferBinding);

    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::image(uint32_t binding, vk::ShaderStageFlags stages)
{
    vk::DescriptorSetLayoutBinding imageBinding {};
    imageBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    imageBinding.stageFlags = stages;
    imageBinding.binding = binding;
    imageBinding.descriptorCount = 1;
    bindings.push_back(imageBinding);

    return *this;
}

DescriptorSet DescriptorSetBuilder::build()
{
    DescriptorSet descriptorSet(engine);

    // Create the layout
    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    vk::DescriptorSetLayout layout = engine->device.createDescriptorSetLayout(layoutInfo);
    descriptorSet.layout = layout;
    descriptorSet.pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(layout);
    });

    // Create descriptor sets using the layout
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSet.layout);
    vk::DescriptorSetAllocateInfo descriptorAllocInfo = {};
    descriptorAllocInfo.descriptorPool = engine->descriptorPool;
    descriptorAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    descriptorAllocInfo.pSetLayouts = layouts.data();
    auto sceneDescriptorAllocResult = engine->device.allocateDescriptorSets(descriptorAllocInfo);
    std::vector<vk::DescriptorSet> localSets;
    descriptorSet.pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.freeDescriptorSets(delEngine->descriptorPool, static_cast<uint32_t>(sceneDescriptorAllocResult.size()), sceneDescriptorAllocResult.data());
    });

    // Copy results into our set storage
    descriptorSet.sets = ResourceRing<vk::DescriptorSet>::fromFunc(MAX_FRAMES_IN_FLIGHT, [&](size_t i) {
        return sceneDescriptorAllocResult[i];
    });

    return descriptorSet;
}
