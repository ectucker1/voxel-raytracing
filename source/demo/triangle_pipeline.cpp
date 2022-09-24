#include "triangle_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/resource/shader_module.hpp"

std::vector<vk::PipelineShaderStageCreateInfo> TrianglePipeline::buildShaderStages()
{
    vertexModule = std::make_unique<ShaderModule>(engine, "../shader/triangle.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = std::make_unique<ShaderModule>(engine, "../shader/triangle.frag.spv", vk::ShaderStageFlagBits::eFragment);

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

vk::PipelineVertexInputStateCreateInfo TrianglePipeline::buildVertexInputInfo()
{
    // No inputs needed for the demo triangle
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo TrianglePipeline::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}
