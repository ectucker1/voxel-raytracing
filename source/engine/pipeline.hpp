#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "util/deletion_queue.hpp"

class Engine;

// An abstract Vulkan graphics pipelines.
// Different configurations should be implemented as subclasses of this base class.
class APipeline
{
public:
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;

protected:
    std::shared_ptr<Engine> _engine;

    DeletionQueue pipelineDeletionQueue;

    std::vector<vk::DynamicState> _dynamicStates;
    vk::PipelineColorBlendAttachmentState _colorBlendAttachment;

public:
    void init(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass);

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() = 0;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() = 0;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() = 0;
    virtual vk::PipelineDynamicStateCreateInfo buildDynamicState();
    virtual vk::PipelineViewportStateCreateInfo buildViewport();
    virtual vk::PipelineRasterizationStateCreateInfo buildRasterizer();
    virtual vk::PipelineColorBlendStateCreateInfo buildColorBlendAttachment();
    virtual vk::PipelineMultisampleStateCreateInfo buildMultisampling();
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout();
};
