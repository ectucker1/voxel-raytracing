#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "util/resource_ring.hpp"

class Engine;

// A renderer to be drawn by the engine.
// This should store application-specific structures, e.g. RenderPipelines, RenderPasses, and Framebuffers.
class ARenderer
{
private:
    bool _initialized = false;
protected:
    std::shared_ptr<Engine> _engine;

    vk::RenderPass _windowRenderPass;
    ResourceRing<vk::Framebuffer> _windowFramebuffers;

public:
    // Called to initialize the renderer.
    // RenderPipelines, RenderPasses, and other data should be initialized here.
    virtual void init(const std::shared_ptr<Engine>& engine);
    // Called each frame to update any logic/internal data for the renderer.
    // delta is the amount of time since the last update.
    virtual void update(float delta) = 0;
    // Called each frame to record rendering commands in the given buffer.
    // flightFrame is the index of the current frame in flight, and should be used to alternate framebuffers.
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t flightFrame) = 0;

    // Returns true if this renderer has been initialized.
    bool initialized() const { return _initialized; }

protected:
    void initWindowRenderPass();
    void initWindowFramebuffers();
};
