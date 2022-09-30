#pragma once

#include "engine/resource.hpp"
#include <vulkan/vulkan.hpp>

// Abstraction around creating Vulkan descriptor sets.
// These should typically be created by pipelines to bind shader uniforms.
// Each DescriptorSet object will implicitly create a descriptor set for each flight frame.
class DescriptorSet : AResource
{
public:
    // The layout of the descriptor set
    vk::DescriptorSetLayout layout;

private:
    // Bindings internally used to build the descriptor set
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    // The actual descriptor set for each frame
    std::vector<vk::DescriptorSet> sets;

public:
    // Creates a new descriptor set.
    // Initialization is not complete until build() is called.
    DescriptorSet(const std::shared_ptr<Engine>& engine) : AResource(engine) {}

    void bindBuffer(uint32_t binding, vk::ShaderStageFlags stages, vk::DescriptorType type);
    void bindImage(uint32_t binding, vk::ShaderStageFlags stages);

    // Builds this descriptor set using the bindings.
    void build();

    // Returns a pointer to the descriptor set for a given frame.
    const vk::DescriptorSet* getSet(uint32_t frame) const;

    // Writes buffer data to the descriptor set for the given frame.
    void writeBuffer(uint32_t binding, uint32_t frame,
                     vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorType type) const;
    // Writes image data to the descriptor set for the given frame.
    void writeImage(uint32_t binding, uint32_t frame,
                    vk::ImageView imageView, vk::Sampler sampler, vk::ImageLayout layout) const;

    // Writes buffer data to the descriptor set for all frames.
    void initBuffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorType type) const;
    // Writes image data to the descriptor set for all frames.
    void initImage(uint32_t binding, vk::ImageView imageView, vk::Sampler sampler, vk::ImageLayout layout) const;
};
