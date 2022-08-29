#include "shader_module.hpp"

#include "engine/engine.hpp"
#include <fstream>
#include <fmt/format.h>

ShaderModule::ShaderModule(const std::shared_ptr<Engine>& engine, const std::string& filename, vk::ShaderStageFlagBits stage) :
    _engine(engine), _filename(filename), _stage(stage) {}

void ShaderModule::load()
{
    // Open shader file
    std::ifstream file(_filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error(fmt::format("Failed to open shader file {}.", _filename));
    }

    // Reserve buffer to load file
    size_t filesize = file.tellg();
    std::vector<uint32_t> buffer(filesize / sizeof(uint32_t));

    // Load file into buffer
    file.seekg(0);
    file.read((char*)buffer.data(), filesize);

    vk::ShaderModuleCreateInfo shaderCreateInfo;
    shaderCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);
    shaderCreateInfo.pCode = buffer.data();
    _shaderModule = _engine->logicalDevice.createShaderModule(shaderCreateInfo);

    _loaded = true;
}

vk::PipelineShaderStageCreateInfo ShaderModule::buildStageCreateInfo() const
{
    vk::PipelineShaderStageCreateInfo info;
    info.stage = _stage;
    info.module = _shaderModule;
    info.pName = "main";
    return info;
}
