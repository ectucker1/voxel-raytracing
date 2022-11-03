#pragma once

#include <optional>
#include "engine/pipeline/pipeline.hpp"
#include "engine/resource/shader_module.hpp"
#include "engine/pipeline/descriptor_set.hpp"
#include <glm/glm.hpp>
#include "voxels/resource/parameters.hpp"

class BlitPipeline : public APipeline
{
public:
    std::optional<DescriptorSet> descriptorSet;

private:
    std::optional<ShaderModule> vertexModule;
    std::optional<ShaderModule> fragmentModule;

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;

    std::optional<vk::PushConstantRange> pushConstantRange;

protected:
    BlitPipeline(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass) : APipeline(engine, pass) {};

public:
    static BlitPipeline build(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass);

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout() override;
};
