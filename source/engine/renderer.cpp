#include "renderer.hpp"

#include "engine/engine.hpp"

ARenderer::ARenderer(const std::shared_ptr<Engine>& engine) : AResource(engine)
{
    initWindowRenderPass();
    initWindowFramebuffers();
}

void ARenderer::initWindowRenderPass()
{
    _windowRenderPass = RenderPassBuilder(engine)
        .color(0, engine->swapchain.imageFormat, glm::vec4(0.0))
        .build();
}

void ARenderer::initWindowFramebuffers()
{
    engine->recreationQueue->push(RecreationEventFlags::WINDOW_RESIZE, [&]() {
        uint32_t imageCount = static_cast<uint32_t>(engine->swapchain.size());
        _windowFramebuffers = ResourceRing<Framebuffer>::fromFunc(imageCount, [&](uint32_t i) {
            return FramebufferBuilder(engine, _windowRenderPass->renderPass, engine->windowSize)
                    .color(engine->swapchain.imageViews[i])
                    .build();
        });

        uint32_t framebufferGroup = engine->deletionQueue.push_group([&]() {
            _windowFramebuffers.destroy([&](const Framebuffer& framebuffer) {
                framebuffer.destroy();
            });
        });

        return [=](const std::shared_ptr<Engine>& delEngine) {
            delEngine->deletionQueue.destroy_group(framebufferGroup);
        };
    });
}
