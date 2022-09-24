#include "voxel_sdf_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/buffer.hpp"
#include "voxels/screen_quad_push.hpp"
#include "voxels/material.hpp"

std::vector<vk::PipelineShaderStageCreateInfo> VoxelSDFPipeline::buildShaderStages()
{
    vertexModule = std::make_unique<ShaderModule>(engine, "../shader/screen_quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = std::make_unique<ShaderModule>(engine, "../shader/voxel_volume.frag.spv", vk::ShaderStageFlagBits::eFragment);

    pipelineDeletionQueue.push_group([&]() {
        vertexModule->destroy();
        fragmentModule->destroy();
    });

    return
    {
        vertexModule->buildStageCreateInfo(),
        fragmentModule->buildStageCreateInfo()
    };
}

vk::PipelineVertexInputStateCreateInfo VoxelSDFPipeline::buildVertexInputInfo()
{
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo VoxelSDFPipeline::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}

vk::PipelineLayoutCreateInfo VoxelSDFPipeline::buildPipelineLayout()
{
    // Screen push constants
    pushConstantRange = std::make_unique<vk::PushConstantRange>();
    pushConstantRange->offset = 0;
    pushConstantRange->size = sizeof(ScreenQuadPush);
    pushConstantRange->stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    // Scene texture descriptor layout
    vk::DescriptorSetLayoutBinding sceneDataBinding {};
    sceneDataBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    sceneDataBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    sceneDataBinding.binding = 0;
    sceneDataBinding.descriptorCount = 1;
    vk::DescriptorSetLayoutCreateInfo sceneDataLayoutInfo {};
    sceneDataLayoutInfo.bindingCount = 1;
    sceneDataLayoutInfo.pBindings = &sceneDataBinding;
    sceneDataLayout = engine->device.createDescriptorSetLayout(sceneDataLayoutInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        engine->device.destroy(sceneDataLayout);
    });

    // Scene texture descriptor set
    std::vector<vk::DescriptorSetLayout> sceneDataLayouts(MAX_FRAMES_IN_FLIGHT, sceneDataLayout);
    vk::DescriptorSetAllocateInfo sceneDescriptorAllocInfo = {};
    sceneDescriptorAllocInfo.descriptorPool = engine->descriptorPool;
    sceneDescriptorAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    sceneDescriptorAllocInfo.pSetLayouts = sceneDataLayouts.data();
    auto sceneDescriptorAllocResult = engine->device.allocateDescriptorSets(sceneDescriptorAllocInfo);
    sceneDataDescriptorSet = sceneDescriptorAllocResult.front();
    descriptorSets.push_back(sceneDataDescriptorSet);
    vk::DescriptorImageInfo imageInfo {};
    imageInfo.sampler = sceneData->sampler;
    imageInfo.imageView = sceneData->imageView;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    vk::WriteDescriptorSet sceneTextureWrite = {};
    sceneTextureWrite.dstBinding = 0;
    sceneTextureWrite.dstSet = sceneDataDescriptorSet;
    sceneTextureWrite.descriptorCount = 1;
    sceneTextureWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    sceneTextureWrite.pImageInfo = &imageInfo;
    engine->device.updateDescriptorSets(1, &sceneTextureWrite, 0, nullptr);

    // Palette descriptor layout
    vk::DescriptorSetLayoutBinding paletteBinding {};
    paletteBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    paletteBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    paletteBinding.binding = 1;
    paletteBinding.descriptorCount = 1;
    vk::DescriptorSetLayoutCreateInfo paletteLayoutInfo {};
    paletteLayoutInfo.bindingCount = 1;
    paletteLayoutInfo.pBindings = &paletteBinding;
    paletteLayout = engine->device.createDescriptorSetLayout(paletteLayoutInfo);
    engine->deletionQueue.push_deletor(deletorGroup, [=]() {
        engine->device.destroy(paletteLayout);
    });

    // Palette descriptor set
    std::vector<vk::DescriptorSetLayout> paletteLayouts(MAX_FRAMES_IN_FLIGHT, paletteLayout);
    vk::DescriptorSetAllocateInfo paletteDescriptorAllocInfo = {};
    paletteDescriptorAllocInfo.descriptorPool = engine->descriptorPool;
    paletteDescriptorAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    paletteDescriptorAllocInfo.pSetLayouts = paletteLayouts.data();
    auto paletteDescriptorAllocResult = engine->device.allocateDescriptorSets(paletteDescriptorAllocInfo);
    paletteDescriptorSet = paletteDescriptorAllocResult.front();
    descriptorSets.push_back(paletteDescriptorSet);
    vk::DescriptorBufferInfo paletteBufferInfo {};
    paletteBufferInfo.buffer = paletteBuffer->buffer;
    paletteBufferInfo.offset = 0;
    paletteBufferInfo.range = 256 * sizeof(Material);
    vk::WriteDescriptorSet paletteBufferWrite = {};
    paletteBufferWrite.dstBinding = 1;
    paletteBufferWrite.dstSet = paletteDescriptorSet;
    paletteBufferWrite.descriptorCount = 1;
    paletteBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    paletteBufferWrite.pBufferInfo = &paletteBufferInfo;
    engine->device.updateDescriptorSets(1, &paletteBufferWrite, 0, nullptr);

    descriptorLayouts.push_back(sceneDataLayout);
    descriptorLayouts.push_back(paletteLayout);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pPushConstantRanges = pushConstantRange.get();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
    layoutInfo.pSetLayouts = descriptorLayouts.data();

    return layoutInfo;
}
