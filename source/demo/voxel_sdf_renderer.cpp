#include "voxel_sdf_renderer.hpp"

#include <stdlib.h>
#include "engine/engine.hpp"
#include "demo/screen_quad_push.hpp"

void VoxelSDFRenderer::init(const std::shared_ptr<Engine>& engine)
{
    ARenderer::init(engine);

    _sceneTexture = std::make_shared<Texture3D>();
    uint8_t sceneData[16 * 16 * 16 * 1];
    for (int i = 0; i < 16 * 16 * 16 * 1; i++)
    {
        sceneData[i] = rand() % 2;
    }
    _sceneTexture->init(_engine, sceneData, 16, 16, 16, 1, vk::Format::eR8Uint);

    _pipeline = VoxelSDFPipeline();
    _pipeline.sceneData = _sceneTexture;
    _pipeline.init(_engine, _windowRenderPass);
    _engine->mainDeletionQueue.push_group([&]() {
       _engine->logicalDevice.destroy(_pipeline.layout);
       _engine->logicalDevice.destroy(_pipeline.pipeline);
    });
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
}

void VoxelSDFRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t flightFrame)
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
    renderpassInfo.renderArea.extent = vk::Extent2D(_engine->windowSize.x, _engine->windowSize.y);
    renderpassInfo.framebuffer = _windowFramebuffers[flightFrame];
    renderpassInfo.clearValueCount = 1;
    renderpassInfo.pClearValues = &clearValue;
    commandBuffer.beginRenderPass(renderpassInfo, vk::SubpassContents::eInline);

    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.pipeline);

    // Set viewport
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_engine->windowSize.x);
    viewport.height = static_cast<float>(_engine->windowSize.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // Set scissor
    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(_engine->windowSize.x, _engine->windowSize.y);
    commandBuffer.setScissor(0, 1, &scissor);

    // Set push constants
    ScreenQuadPush constants;
    constants.screenSize = _engine->windowSize;
    constants.time = _time;
    commandBuffer.pushConstants(_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);

    // Bind texture descriptor set
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline.layout,
                                     0, 1, &_pipeline.sceneDataDescriptorSet,
                                     0, nullptr);

    commandBuffer.draw(3, 1, 0, 0);

    // End main renderpass
    commandBuffer.endRenderPass();
}
