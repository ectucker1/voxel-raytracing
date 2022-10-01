#pragma once

#include "engine/resource.hpp"
#include "engine/resource_builder.hpp"
#include "util/resource_ring.hpp"
#include <vulkan/vulkan.hpp>

// Abstraction around creating Vulkan descriptor sets.
// These should typically be created by pipelines to bind shader uniforms.
// Each DescriptorSet object will implicitly create a descriptor set for each flight frame.
class DescriptorSet : AResource
{
public:
    // The layout of the descriptor set
    vk::DescriptorSetLayout layout;
    // The actual descriptor set for each frame
    ResourceRing<vk::DescriptorSet> sets;

protected:
    // Creates a new descriptor set.
    // Initialization will be completed by the friend builder.
    explicit DescriptorSet(const std::shared_ptr<Engine>& engine) : AResource(engine) {}

    friend class DescriptorSetBuilder;

public:
    // Returns a pointer to the next descriptor set for a given frame.
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

// Builder for descriptor sets.
class DescriptorSetBuilder : ResourceBuilder<DescriptorSet>
{
private:
    // Bindings internally used to build the descriptor set
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

public:
    // Begins the descriptor set builder.
    explicit DescriptorSetBuilder(const std::shared_ptr<Engine>& engine) : ResourceBuilder<DescriptorSet>(engine) {}

    // Adds a buffer descriptor at the given binding.
    DescriptorSetBuilder& buffer(uint32_t binding, vk::ShaderStageFlags stages, vk::DescriptorType type);
    // Adds an image descriptor at the given binding.
    DescriptorSetBuilder& image(uint32_t binding, vk::ShaderStageFlags stages);

    // Builds the descriptor set.
    DescriptorSet build() override;
};
