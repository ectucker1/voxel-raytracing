#pragma once

#include <memory>
#include <optional>
#include "engine/pipeline/pipeline.hpp"
#include "engine/resource/shader_module.hpp"
#include "util/resource_ring.hpp"
#include "engine/pipeline/descriptor_set.hpp"

class VoxelSDFPipeline : public APipeline
{
public:
    std::optional<DescriptorSet> descriptorSet;

private:
    std::optional<ShaderModule> vertexModule;
    std::optional<ShaderModule> fragmentModule;

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;

    std::optional<vk::PushConstantRange> pushConstantRange;

protected:
    VoxelSDFPipeline(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass) : APipeline(engine, pass) {};

public:
    static VoxelSDFPipeline build(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass);

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout() override;
    virtual vk::PipelineColorBlendStateCreateInfo buildColorBlendAttachment() override;
};
