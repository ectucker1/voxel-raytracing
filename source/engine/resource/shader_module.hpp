#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include "engine/resource.hpp"

class Engine;

class ShaderModule : public AResource
{
private:
    vk::ShaderStageFlagBits _stage;
    vk::ShaderModule _shaderModule;

public:
    ShaderModule(const std::shared_ptr<Engine>& engine, const std::string& filename, vk::ShaderStageFlagBits stage);

    vk::PipelineShaderStageCreateInfo buildStageCreateInfo() const;
};
