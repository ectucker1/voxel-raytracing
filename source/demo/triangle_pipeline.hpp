#pragma once

#include <memory>
#include "engine/pipeline_builder.hpp"
#include "engine/shader_module.hpp"

class TrianglePipelineBuilder
    : public APipelineBuilder
{
private:
    std::unique_ptr<ShaderModule> vertexModule;
    std::unique_ptr<ShaderModule> fragmentModule;
public:
    TrianglePipelineBuilder(const std::shared_ptr<Engine>& engine);
protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
};
