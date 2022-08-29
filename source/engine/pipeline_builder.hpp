#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

class Engine;

// Builder utility for Vulkan graphics pipelines.
// Different configurations should be implemented as subclasses of this base class.
class APipelineBuilder
{
protected:
    std::shared_ptr<Engine> _engine;

    vk::Viewport _viewport;
    vk::Rect2D _scissor;
    vk::PipelineColorBlendAttachmentState _colorBlendAttachment;

public:
    APipelineBuilder(const std::shared_ptr<Engine>& engine);
    vk::Pipeline build(const vk::RenderPass& pass);

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() = 0;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() = 0;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() = 0;
    virtual vk::PipelineViewportStateCreateInfo buildViewport();
    virtual vk::PipelineRasterizationStateCreateInfo buildRasterizer();
    virtual vk::PipelineColorBlendStateCreateInfo buildColorBlendAttachment();
    virtual vk::PipelineMultisampleStateCreateInfo buildMultisampling();
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout();
};
