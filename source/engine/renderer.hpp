#pragma once

#include <optional>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "util/resource_ring.hpp"
#include "engine/resource.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/pipeline/framebuffer.hpp"

class Engine;

// A renderer to be drawn by the engine.
// This should store application-specific structures, e.g. RenderPipelines, RenderPasses, and Framebuffers.
class ARenderer : public AResource
{
protected:
    std::unique_ptr<RenderPass> _windowRenderPass;
    ResourceRing<Framebuffer> _windowFramebuffers;

public:
    // Called to initialize the renderer.
    // RenderPipelines, RenderPasses, and other data should be initialized here.
    explicit ARenderer(const std::shared_ptr<Engine>& engine);
    // Called each frame to update any logic/internal data for the renderer.
    // delta is the amount of time since the last update.
    virtual void update(float delta) = 0;
    // Called each frame to record rendering commands in the given buffer.
    // flightFrame is the index of the current frame in flight, and should be used to alternate framebuffers.
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) = 0;

protected:
    void initWindowRenderPass();
    void initWindowFramebuffers();
};
