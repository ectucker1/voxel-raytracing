#include "voxel_sdf_pipeline.hpp"

#include "engine/engine.hpp"
#include "engine/shader_module.hpp"
#include "demo/screen_quad_push.hpp"

VoxelSDFPipleineBuilder::VoxelSDFPipleineBuilder(const std::shared_ptr<Engine>& engine) : APipelineBuilder(engine) {}

std::vector<vk::PipelineShaderStageCreateInfo> VoxelSDFPipleineBuilder::buildShaderStages()
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

vk::PipelineVertexInputStateCreateInfo VoxelSDFPipleineBuilder::buildVertexInputInfo()
{
    // No inputs needed for the fullscreen triangle
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    return vertexInputInfo;
}

vk::PipelineInputAssemblyStateCreateInfo VoxelSDFPipleineBuilder::buildInputAssembly()
{
    // Triangle list with no restart
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyInfo.primitiveRestartEnable = false;
    return inputAssemblyInfo;
}

vk::PipelineLayoutCreateInfo VoxelSDFPipleineBuilder::buildPipelineLayout()
{
    pushConstantRange = std::make_unique<vk::PushConstantRange>();
    pushConstantRange->offset = 0;
    pushConstantRange->size = sizeof(ScreenQuadPush);
    pushConstantRange->stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::PipelineLayoutCreateInfo layoutInfo;
    layoutInfo.pPushConstantRanges = pushConstantRange.get();
    layoutInfo.pushConstantRangeCount = 1;

    return layoutInfo;
}
