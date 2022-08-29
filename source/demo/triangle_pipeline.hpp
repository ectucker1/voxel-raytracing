#pragma once

#include "engine/pipeline_builder.hpp"

class TrianglePipelineBuilder
    : public APipelineBuilder
{
public:
    TrianglePipelineBuilder(const std::shared_ptr<Engine>& engine);
protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
};
