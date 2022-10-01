#include "triangle_renderer.hpp"

#include "engine/engine.hpp"

TriangleRenderer::TriangleRenderer(const std::shared_ptr<Engine>& engine) : ARenderer(engine)
{
    _pipeline = std::make_unique<TrianglePipeline>(engine, _windowRenderPass);
    _pipeline->buildAll();
}

void TriangleRenderer::update(float delta)
{
    _time += delta;
}

void TriangleRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t)
{
    // Create clear color for this frame
    vk::ClearValue clearValue;
    float flash = abs(sin(_time));
    clearValue.color = vk::ClearColorValue(std::array<float, 4> {0.0f, 0.0f, flash, 1.0f});

    // Start main renderpass
    vk::RenderPassBeginInfo renderpassInfo;
    renderpassInfo.renderPass = _windowRenderPass;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.offset = 0;
    renderpassInfo.renderArea.extent = vk::Extent2D(engine->windowSize.x, engine->windowSize.y);
    renderpassInfo.framebuffer = _windowFramebuffers[swapchainImage];
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues = &clearValue;
    commandBuffer.beginRenderPass(renderpassInfo, vk::SubpassContents::eInline);

    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline->pipeline);

    // Set viewport
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(engine->windowSize.x);
    viewport.height = static_cast<float>(engine->windowSize.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // Set scissor
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(engine->windowSize.x, engine->windowSize.y);
    commandBuffer.setScissor(0, 1, &scissor);

    commandBuffer.draw(3, 1, 0, 0);

    // End main renderpass
    commandBuffer.endRenderPass();
}
