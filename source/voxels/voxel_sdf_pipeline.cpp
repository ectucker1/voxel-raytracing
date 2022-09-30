#include "voxel_sdf_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"
#include "engine/resource/buffer.hpp"
#include "voxels/screen_quad_push.hpp"

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

    // Shader uniforms
    descriptorSet.bindImage(0, vk::ShaderStageFlagBits::eFragment);
    descriptorSet.bindBuffer(1, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer);
    descriptorSet.bindImage(2, vk::ShaderStageFlagBits::eFragment);
    descriptorSet.build();

    // Actual layout
    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pPushConstantRanges = pushConstantRange.get();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSet.layout;

    return layoutInfo;
}
