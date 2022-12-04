#include "blit_stage.hpp"

#include "engine/resource/buffer.hpp"
#include "voxels/pipeline/blit_pipeline.hpp"
#include "engine/engine.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "engine/commands/command_util.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/resource/render_image.hpp"

BlitStage::BlitStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings, const RenderPass& renderPass) : AVoxelRenderStage(engine, settings)
{
    _offsetsBuffer = std::make_unique<Buffer>(engine, sizeof(BlitOffsets), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, "Blit Offsets Buffer");
    _offsetsBuffer->copyData(&_offsets, sizeof(BlitOffsets));

    _pipeline = std::make_unique<BlitPipeline>(BlitPipeline::build(engine, renderPass.renderPass));

    engine->recreationQueue->push(RecreationEventFlags::TARGET_RESIZE, [&]() {
        _pipeline->descriptorSet->initBuffer(1, _offsetsBuffer->buffer, _offsetsBuffer->size, vk::DescriptorType::eUniformBuffer);

        return [=](const std::shared_ptr<Engine>&) {};
    });
}

void BlitStage::record(const vk::CommandBuffer& cmd, uint32_t flightFrame, const RenderImage& image, const Framebuffer& windowFramebuffer, const RenderPass& windowRenderPass, const std::function<void(const vk::CommandBuffer&)> uiStage)
{
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(engine->windowSize.x);
    viewport.height = static_cast<float>(engine->windowSize.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(engine->windowSize.x, engine->windowSize.y);
    cmd.setScissor(0, 1, &scissor);

    _offsets.sourceSize = { _settings->targetResolution.x, _settings->targetResolution.y };
    _offsets.targetSize = engine->windowSize;
    _offsetsBuffer->copyData(&_offsets, sizeof(BlitOffsets));

    _pipeline->descriptorSet->writeImage(0, flightFrame, image.imageView, image.sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    _pipeline->descriptorSet->writeBuffer(1, flightFrame, _offsetsBuffer->buffer, sizeof(BlitOffsets), vk::DescriptorType::eUniformBuffer);

    windowRenderPass.recordBegin(cmd, windowFramebuffer);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline->pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->layout,
        0, 1,
        _pipeline->descriptorSet->getSet(flightFrame),
        0, nullptr);
    cmd.draw(3, 1, 0, 0);
    uiStage(cmd);
    cmd.endRenderPass();
}