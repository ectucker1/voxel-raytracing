#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>

class Engine;

class ShaderModule {
private:
    std::shared_ptr<Engine> _engine;

    std::string _filename = "";
    vk::ShaderStageFlagBits _stage;
    vk::ShaderModule _shaderModule;

    bool _loaded = false;

public:
    ShaderModule(const std::shared_ptr<Engine>& engine, const std::string& filename, vk::ShaderStageFlagBits stage);

    void load();
    bool isLoaded() const { return _loaded; }

    vk::PipelineShaderStageCreateInfo buildStageCreateInfo() const;
};
