#pragma once

#include <optional>
#include "engine/resource.hpp"
#include "engine/resource_builder.hpp"
#include <vulkan/vulkan.hpp>

class Framebuffer;

// Abstraction around a Vulkan render pass.
class RenderPass : public AResource
{
public:
    // The actual render pass.
    vk::RenderPass renderPass;

    // The default values to clear to
    std::vector<vk::ClearValue> defaultClears;

protected:
    // Creates a new render pass.
    // Initialization will be completed by the friend builder.
    explicit RenderPass(const std::shared_ptr<Engine>& engine) : AResource(engine) {}

    friend class RenderPassBuilder;

public:
    // Records commands to begin this render pass, including the default clear colors.
    void recordBegin(const vk::CommandBuffer& cmd, const Framebuffer& framebuffer) const;
};

// Builder for render passes.
class RenderPassBuilder : ResourceBuilder<RenderPass>
{
private:
    // Color attachments for this render pass.
    std::vector<vk::AttachmentDescription> colorAttachments;
    // Color attachment references for this render pass.
    std::vector<vk::AttachmentReference> colorAttachmentRefs;
    // Color attachment clear values for this render pass.
    std::vector<vk::ClearValue> colorClears;

    // Depth stencil attachment for this render pass.
    std::optional<vk::AttachmentDescription> depthStencilAttachment;
    // Depth stencil attachment ref for this render pass/
    std::optional<vk::AttachmentReference> depthStencilAttachmentRef;
    // Depth stencil attachment clear values for this render pass.
    std::optional<vk::ClearValue> depthStencilClear;

public:
    // Begins the render pass builder.
    explicit RenderPassBuilder(const std::shared_ptr<Engine>& engine) : ResourceBuilder<RenderPass>(engine) {}

    // Adds a color attachment at the given location.
    RenderPassBuilder& color(uint32_t attachment, vk::Format format, glm::vec4 clear);
    // Adds a depth stencil attachment at the given location.
    RenderPassBuilder& depthStencil(uint32_t attachment, vk::Format format, float depthClear, uint32_t stencilClear);

    // Builds the render pass.
    RenderPass build() override;
};
