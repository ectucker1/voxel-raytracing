#include "pipeline.hpp"

#include "engine/engine.hpp"

APipeline::APipeline(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass)
    : AResource(engine), pass(pass) {}

void APipeline::buildAll()
{
    // Create prerequisite pipeline infos
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = buildShaderStages();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = buildVertexInputInfo();
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = buildInputAssembly();
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo = buildDynamicState();
    vk::PipelineViewportStateCreateInfo viewportInfo = buildViewport();
    vk::PipelineRasterizationStateCreateInfo rasterizationInfo = buildRasterizer();
    vk::PipelineMultisampleStateCreateInfo multisamplingInfo = buildMultisampling();
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo = buildColorBlendAttachment();
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = buildDepthStencil();
    vk::PipelineLayoutCreateInfo layoutInfo = buildPipelineLayout();

    // Create layout
    vk::PipelineLayout createdLayout = engine->device.createPipelineLayout(layoutInfo);
    layout = createdLayout;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdLayout);
    });

    // Group together create info
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;

    // Actually build the pipeline
    auto pipelineResult = engine->device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    vk::resultCheck(pipelineResult.result, "Error creating graphics pipeline");
    vk::Pipeline createdPipeline = pipelineResult.value;
    pipeline = createdPipeline;
    pushDeletor([=](const std::shared_ptr<Engine>& delEngine) {
        delEngine->device.destroy(createdPipeline);
    });

    pipelineDeletionQueue.destroy_all();
}

vk::PipelineDynamicStateCreateInfo APipeline::buildDynamicState()
{
    // Use viewport and scissor as dynamic states so we can resize the window
    _dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_dynamicStates.size());
    dynamicStateInfo.pDynamicStates = _dynamicStates.data();
    return dynamicStateInfo;
}

vk::PipelineViewportStateCreateInfo APipeline::buildViewport()
{
    vk::PipelineViewportStateCreateInfo viewportInfo;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    return viewportInfo;
}

vk::PipelineRasterizationStateCreateInfo APipeline::buildRasterizer()
{
    vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
    rasterizationInfo.depthClampEnable = false;
    rasterizationInfo.rasterizerDiscardEnable = false;
    rasterizationInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationInfo.lineWidth = 1.0f;
    rasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizationInfo.depthBiasEnable = false;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;

    return rasterizationInfo;
}

vk::PipelineColorBlendStateCreateInfo APipeline::buildColorBlendAttachment()
{
    // Attach to all color bits
    _colorBlendAttachment = vk::PipelineColorBlendAttachmentState();
    _colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;
    _colorBlendAttachment.blendEnable = false;

    // No blending ops needed
    vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
    colorBlendInfo.logicOpEnable = false;
    colorBlendInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &_colorBlendAttachment;

    return colorBlendInfo;
}

vk::PipelineDepthStencilStateCreateInfo APipeline::buildDepthStencil()
{
    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
    depthStencilInfo.depthTestEnable = false;
    depthStencilInfo.depthWriteEnable = true;
    depthStencilInfo.depthBoundsTestEnable = false;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = false;
    return depthStencilInfo;
}

vk::PipelineMultisampleStateCreateInfo APipeline::buildMultisampling()
{
    // No multisampling for now
    vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
    multisamplingInfo.sampleShadingEnable = false;
    multisamplingInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisamplingInfo.minSampleShading = 1.0f;
    multisamplingInfo.pSampleMask = nullptr;
    multisamplingInfo.alphaToCoverageEnable = false;
    multisamplingInfo.alphaToOneEnable = false;

    return multisamplingInfo;
}

vk::PipelineLayoutCreateInfo APipeline::buildPipelineLayout()
{
    // Defaults are fine for layout
    return {};
}
