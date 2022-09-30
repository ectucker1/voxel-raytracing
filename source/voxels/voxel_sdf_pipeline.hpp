#pragma once

#include <memory>
#include "engine/pipeline.hpp"
#include "engine/resource/shader_module.hpp"
#include "util/resource_ring.hpp"
#include "engine/descriptor_set.hpp"

class Texture3D;
class Texture2D;
class Buffer;

class VoxelSDFPipeline : public APipeline
{
public:
    DescriptorSet descriptorSet;

private:
    std::unique_ptr<ShaderModule> vertexModule;
    std::unique_ptr<ShaderModule> fragmentModule;
    std::unique_ptr<vk::PushConstantRange> pushConstantRange;

public:
    VoxelSDFPipeline(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass)
                     : APipeline(engine, pass), descriptorSet(engine) {};

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout() override;
};
