#include "denoiser_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"
#include "voxels/resource/screen_quad_push.hpp"

DenoiserPipeline DenoiserPipeline::build(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass)
{
    DenoiserPipeline pipeline(engine, pass);
    pipeline.buildAll();
    return pipeline;
}

std::vector<vk::PipelineShaderStageCreateInfo> DenoiserPipeline::buildShaderStages()
{
    vertexModule = ShaderModule(engine, "../shader/screen_quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = ShaderModule(engine, "../shader/denoiser.frag.spv", vk::ShaderStageFlagBits::eFragment);

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

vk::PipelineVertexInputStateCreateInfo DenoiserPipeline::buildVertexInputInfo()
{
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo DenoiserPipeline::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}

vk::PipelineLayoutCreateInfo DenoiserPipeline::buildPipelineLayout()
{
    // Screen push constants
    pushConstantRange = vk::PushConstantRange();
    pushConstantRange->offset = 0;
    pushConstantRange->size = sizeof(ScreenQuadPush);
    pushConstantRange->stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    // Shader uniforms
    auto localDescriptorSet = DescriptorSetBuilder(engine)
        .image(0, vk::ShaderStageFlagBits::eFragment)
        .image(1, vk::ShaderStageFlagBits::eFragment)
        .image(2, vk::ShaderStageFlagBits::eFragment)
        .buffer(3, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .buffer(4, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .buffer(5, vk::ShaderStageFlagBits::eFragment, vk::DescriptorType::eUniformBuffer)
        .build("Denoiser Descriptor Set");
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
