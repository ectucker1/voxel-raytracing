#include "voxel_sdf_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/shader_module.hpp"
#include "demo/screen_quad_push.hpp"
#include "engine/resource/texture_3d.hpp"

std::vector<vk::PipelineShaderStageCreateInfo> VoxelSDFPipeline::buildShaderStages()
{
    vertexModule = std::make_unique<ShaderModule>(_engine, "../shader/screen_quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = std::make_unique<ShaderModule>(_engine, "../shader/voxel_sdf_toy.frag.spv", vk::ShaderStageFlagBits::eFragment);

    vertexModule->load();
    fragmentModule->load();

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
    // No inputs needed for the fullscreen triangle
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
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
    pushConstantRange->stageFlags = vk::ShaderStageFlagBits::eFragment;

    // Texture descriptor
    vk::DescriptorSetLayoutBinding sceneDataBinding {};
    sceneDataBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    sceneDataBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    sceneDataBinding.binding = 0;
    sceneDataBinding.descriptorCount = 1;
    vk::DescriptorSetLayoutCreateInfo sceneDataLayoutInfo {};
    sceneDataLayoutInfo.bindingCount = 1;
    sceneDataLayoutInfo.pBindings = &sceneDataBinding;
    sceneDataLayout = _engine->logicalDevice.createDescriptorSetLayout(sceneDataLayoutInfo);
    _engine->mainDeletionQueue.push_group([=]() {
        _engine->logicalDevice.destroy(sceneDataLayout);
    });

    // Descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, sceneDataLayout);
    vk::DescriptorSetAllocateInfo sceneDescriptorAllocInfo = {};
    sceneDescriptorAllocInfo.descriptorPool = _engine->descriptorPool;
    sceneDescriptorAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    sceneDescriptorAllocInfo.pSetLayouts = layouts.data();
    auto sceneDescriptorAllocResult = _engine->logicalDevice.allocateDescriptorSets(sceneDescriptorAllocInfo);
    sceneDataDescriptorSet = sceneDescriptorAllocResult.front();

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

    _engine->logicalDevice.updateDescriptorSets(1, &sceneTextureWrite, 0, nullptr);

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pPushConstantRanges = pushConstantRange.get();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &sceneDataLayout;

    return layoutInfo;
}
