#pragma once

#include <optional>
#include "engine/resource.hpp"
#include "engine/resource_builder.hpp"
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

// Wrapper around a Vulkan framebuffer.
// A framebuffer connects some number of image views to a render pass.
class Framebuffer : public AResource
{
public:
    // The actual Vulkan framebuffer.
    vk::Framebuffer framebuffer;

    // The size of the render target.
    glm::uvec2 size;

protected:
    Framebuffer(const std::shared_ptr<Engine>& engine): AResource(engine) {}

    friend class FramebufferBuilder;
};

// Builder for framebuffers.
class FramebufferBuilder : ResourceBuilder<Framebuffer>
{
private:
    // The render pass this buffer connects to.
    vk::RenderPass pass;

    // The size of the render target to connect to.
    glm::uvec2 size;

    // Color attachments for this framebuffer.
    std::vector<vk::ImageView> colorAttachments;

    // Depth stencil attachment for this framebuffer.
    std::optional<vk::ImageView> depthStencilAttachment;

public:
    // Begins the framebuffer builder.
    FramebufferBuilder(const std::shared_ptr<Engine>& engine, vk::RenderPass pass, glm::uvec2 size)
        : ResourceBuilder<Framebuffer>(engine), pass(pass), size(size) {}

    // Adds a color attachment.
    FramebufferBuilder& color(vk::ImageView imageView);
    // Adds a depth stencil attachment.
    FramebufferBuilder& depthStencil(vk::ImageView imageView);

    // Builds the render pass.
    Framebuffer build(const std::string& name) override;
};
