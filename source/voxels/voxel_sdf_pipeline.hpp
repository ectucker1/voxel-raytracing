#pragma once

#include <memory>
#include "engine/pipeline.hpp"
#include "engine/shader_module.hpp"
#include "util/resource_ring.hpp"

class Texture3D;

class VoxelSDFPipeline : public APipeline
{
public:
    std::shared_ptr<Texture3D> sceneData;
    vk::DescriptorSet sceneDataDescriptorSet;

private:
    std::unique_ptr<ShaderModule> vertexModule;
    std::unique_ptr<ShaderModule> fragmentModule;
    std::unique_ptr<vk::PushConstantRange> pushConstantRange;

    vk::DescriptorSetLayout sceneDataLayout;

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout() override;
};
