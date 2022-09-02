#include "pipeline_builder.hpp"

#include "engine/engine.hpp"

APipelineBuilder::APipelineBuilder(const std::shared_ptr<Engine>& engine) : _engine(engine) {}

PipelineStorage APipelineBuilder::build(const vk::RenderPass& pass)
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
    vk::PipelineLayoutCreateInfo layoutInfo = buildPipelineLayout();

    vk::PipelineLayout layout = _engine->logicalDevice.createPipelineLayout(layoutInfo);

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
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;

    // Actually build the pipeline
    auto pipelineResult = _engine->logicalDevice.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    vk::resultCheck(pipelineResult.result, "Error creating graphics pipeline");

    pipelineDeletionQueue.destroy_all();

    return
    {
        layout,
        pipelineResult.value
    };
}

vk::PipelineDynamicStateCreateInfo APipelineBuilder::buildDynamicState()
{
    // Use viewport and scissor as dynamic states so we can resize the window
    _dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_dynamicStates.size());
    dynamicStateInfo.pDynamicStates = _dynamicStates.data();
    return dynamicStateInfo;
}

vk::PipelineViewportStateCreateInfo APipelineBuilder::buildViewport()
{
    vk::PipelineViewportStateCreateInfo viewportInfo;
    viewportInfo.viewportCount = 1;
    viewportInfo.scissorCount = 1;

    return viewportInfo;
}

vk::PipelineRasterizationStateCreateInfo APipelineBuilder::buildRasterizer()
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

vk::PipelineColorBlendStateCreateInfo APipelineBuilder::buildColorBlendAttachment()
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

vk::PipelineMultisampleStateCreateInfo APipelineBuilder::buildMultisampling()
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

vk::PipelineLayoutCreateInfo APipelineBuilder::buildPipelineLayout()
{
    // Defaults are fine for layout
    return {};
}
