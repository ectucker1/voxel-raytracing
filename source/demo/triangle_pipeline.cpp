#include "triangle_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/shader_module.hpp"

TrianglePipelineBuilder::TrianglePipelineBuilder(const std::shared_ptr<Engine>& engine) : APipelineBuilder(engine) {}

std::vector<vk::PipelineShaderStageCreateInfo> TrianglePipelineBuilder::buildShaderStages()
{
    vertexModule = std::make_unique<ShaderModule>(_engine, "../shader/triangle.vert.spv", vk::ShaderStageFlagBits::eVertex);
    fragmentModule = std::make_unique<ShaderModule>(_engine, "../shader/triangle.frag.spv", vk::ShaderStageFlagBits::eFragment);

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

vk::PipelineVertexInputStateCreateInfo TrianglePipelineBuilder::buildVertexInputInfo()
{
    // No inputs needed for the demo triangle
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo TrianglePipelineBuilder::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}
