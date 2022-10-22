#include "blit_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"
#include "voxels/resource/screen_quad_push.hpp"

BlitPipeline BlitPipeline::build(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass)
{
    BlitPipeline pipeline(engine, pass);
    pipeline.buildAll();
    return pipeline;
}

std::vector<vk::PipelineShaderStageCreateInfo> BlitPipeline::buildShaderStages()
{
    vertexModule = ShaderModule(engine, "../shader/screen_quad.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = ShaderModule(engine, "../shader/blit.frag.spv", vk::ShaderStageFlagBits::eFragment);

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

vk::PipelineVertexInputStateCreateInfo BlitPipeline::buildVertexInputInfo()
{
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo BlitPipeline::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}

vk::PipelineLayoutCreateInfo BlitPipeline::buildPipelineLayout()
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
