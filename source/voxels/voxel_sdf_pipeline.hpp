#pragma once

#include <memory>
#include "engine/pipeline.hpp"
#include "engine/resource/shader_module.hpp"
#include "util/resource_ring.hpp"

class Texture3D;
class Texture2D;
class Buffer;

class VoxelSDFPipeline : public APipeline
{
public:
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::DescriptorSet sceneDataDescriptorSet;
    vk::DescriptorSet paletteDescriptorSet;
    vk::DescriptorSet blueNoiseDescriptorSet;

private:
    std::shared_ptr<Texture3D> sceneData;
    std::shared_ptr<Buffer> paletteBuffer;
    std::shared_ptr<Texture2D> blueNoise;

    std::unique_ptr<ShaderModule> vertexModule;
    std::unique_ptr<ShaderModule> fragmentModule;
    std::unique_ptr<vk::PushConstantRange> pushConstantRange;

    std::vector<vk::DescriptorSetLayout> descriptorLayouts;
    vk::DescriptorSetLayout sceneDataLayout;
    vk::DescriptorSetLayout paletteLayout;
    vk::DescriptorSetLayout blueNoiseLayout;

public:
    VoxelSDFPipeline(const std::shared_ptr<Engine>& engine, const vk::RenderPass& pass,
                     const std::shared_ptr<Texture3D>& sceneData, const std::shared_ptr<Buffer>& paletteBuffer,
                     const std::shared_ptr<Texture2D>& blueNoise)
                     : APipeline(engine, pass), sceneData(sceneData), paletteBuffer(paletteBuffer), blueNoise(blueNoise) {};

protected:
    virtual std::vector<vk::PipelineShaderStageCreateInfo> buildShaderStages() override;
    virtual vk::PipelineVertexInputStateCreateInfo buildVertexInputInfo() override;
    virtual vk::PipelineInputAssemblyStateCreateInfo buildInputAssembly() override;
    virtual vk::PipelineLayoutCreateInfo buildPipelineLayout() override;
};
