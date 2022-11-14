#include "voxel_sdf_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"
#include "voxels/resource/screen_quad_push.hpp"

VoxelSDFPipeline VoxelSDFPipeline::build(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass)
{
    VoxelSDFPipeline pipeline(engine, pass);
    pipeline.buildAll();
    return pipeline;
}

std::vector<vk::PipelineShaderStageCreateInfo> VoxelSDFPipeline::buildShaderStages()
{
    vertexModule = ShaderModule(engine, "../shader/screen_quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = ShaderModule(engine, "../shader/voxel_volume.frag.spv", vk::ShaderStageFlagBits::eFragment);

    pipelineDeletionQueue.push_group([=]() {
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
    pushConstantRange = vk::PushConstantRange();
    pushConstantRange->offset = 0;
    pushConstantRange->size = sizeof(ScreenQuadPush);
    pushConstantRange->stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    // Shader uniforms
    auto localDescriptorSet = DescriptorSetBuilder(engine)
        .image(0, vk::ShaderStageFlagBits::eFragment)
        .buffer(1, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .image(2, vk::ShaderStageFlagBits::eFragment)
        .image(3, vk::ShaderStageFlagBits::eFragment)
        .buffer(4, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .buffer(5, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .build();
    descriptorSet = localDescriptorSet;
    pushDeletor([=](const std::shared_ptr<Engine>&) {
        localDescriptorSet.destroy();
    });

    // Actual layout
    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pPushConstantRanges = &pushConstantRange.value();
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSet->layout;

    return layoutInfo;
}

vk::PipelineColorBlendStateCreateInfo VoxelSDFPipeline::buildColorBlendAttachment()
{
    // Attach to all color bits
    colorBlendAttachments = {};

    vk::PipelineColorBlendAttachmentState blendState;
    blendState.colorWriteMask = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;
    blendState.blendEnable = false;
    for (size_t i = 0; i < 6; i++)
        colorBlendAttachments.push_back(blendState);

    // No blending ops needed
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
    colorBlendInfo.logicOpEnable = false;
    colorBlendInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlendInfo.pAttachments = colorBlendAttachments.data();

    return colorBlendInfo;
}
