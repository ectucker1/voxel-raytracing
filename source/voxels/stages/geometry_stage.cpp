#include "geometry_stage.hpp"

#include "engine/resource/buffer.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/engine.hpp"
#include "voxels/voxel_render_settings.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "voxels/pipeline/voxel_sdf_pipeline.hpp"
#include "voxels/resource/voxel_scene.hpp"
#include "engine/resource/texture_2d.hpp"
#include "engine/commands/command_util.hpp"

GeometryStage::GeometryStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings, const std::shared_ptr<VoxelScene>& scene, const std::shared_ptr<Texture2D>& noise) : AVoxelRenderStage(engine, settings), _scene(scene)
{
    _parametersBuffer = std::make_unique<Buffer>(engine, sizeof(VolumeParameters), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _parametersBuffer->copyData(&_parameters, sizeof(VolumeParameters));

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        glm::uvec2 renderRes = settings->renderResolution();

        _colorTarget = std::make_unique<RenderImage>(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
        _depthTarget = std::make_unique<RenderImage>(engine, renderRes.x, renderRes.y, vk::Format::eR32Sfloat,
                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
        _motionTarget = std::make_unique<RenderImage>(engine, renderRes.x, renderRes.y, vk::Format::eR32G32Sfloat,
                                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
        _maskTarget = std::make_unique<RenderImage>(engine, renderRes.x, renderRes.y, vk::Format::eR8Unorm,
                                  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
        _positionTargets = ResourceRing<RenderImage>::fromArgs(2, engine, renderRes.x, renderRes.y, vk::Format::eR32G32B32A32Sfloat,
                                                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
        _normalTarget = std::make_unique<RenderImage>(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Snorm,
                                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);

        return [=](const std::shared_ptr<Engine>&) {
            _colorTarget->destroy();
            _depthTarget->destroy();
            _maskTarget->destroy();
            _maskTarget->destroy();
            _positionTargets.destroy([=](const RenderImage& image) {
                image.destroy();
            });
            _normalTarget->destroy();
        };
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _renderPass = RenderPassBuilder(engine)
            .color(0, _colorTarget->format, glm::vec4(0.0))
            .color(1, _depthTarget->format, glm::vec4(0.0))
            .color(2, _motionTarget->format, glm::vec4(0.0))
            .color(3, _maskTarget->format, glm::vec4(0.0))
            .color(4, _positionTargets[0].format, glm::vec4(0.0))
            .color(5, _normalTarget->format, glm::vec4(0.0))
            .buildUnique();

        return [=](const std::shared_ptr<Engine>&) {
            _renderPass->destroy();
        };
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _framebuffers = ResourceRing<Framebuffer>::fromFunc(2, [&](uint32_t n) {
            return FramebufferBuilder(engine, _renderPass->renderPass, settings->renderResolution())
                .color(_colorTarget->imageView)
                .color(_depthTarget->imageView)
                .color(_motionTarget->imageView)
                .color(_maskTarget->imageView)
                .color(_positionTargets[n].imageView)
                .color(_normalTarget->imageView)
                .build();
        });

        return [=](const std::shared_ptr<Engine>&) {
            _framebuffers.destroy([&](const Framebuffer& framebuffer) {
                framebuffer.destroy();
            });
        };
    });

    _pipeline = std::make_unique<VoxelSDFPipeline>(VoxelSDFPipeline::build(engine, _renderPass->renderPass));

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE, [&]() {
        _pipeline->descriptorSet->initImage(0, scene->sceneTexture->imageView, scene->sceneTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        _pipeline->descriptorSet->initBuffer(1, scene->paletteBuffer->buffer, scene->paletteBuffer->size, vk::DescriptorType::eUniformBuffer);
        _pipeline->descriptorSet->initImage(2, noise->imageView, noise->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        _pipeline->descriptorSet->initBuffer(5, scene->lightBuffer->buffer, scene->lightBuffer->size, vk::DescriptorType::eUniformBuffer);
        _pipeline->descriptorSet->initImage(6, scene->skyboxTexture->imageView, scene->skyboxTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);

        return [=](const std::shared_ptr<Engine>&) {};
    });
}

GeometryBuffer GeometryStage::record(const vk::CommandBuffer& cmd, uint32_t flightFrame)
{
    uint32_t altFrame = (flightFrame + 1) % 2;
    cmdutil::imageMemoryBarrier(
        cmd,
        _positionTargets[flightFrame].image,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor);
    cmdutil::imageMemoryBarrier(
        cmd,
        _positionTargets[altFrame].image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    // Start color renderpass
    _renderPass->recordBegin(cmd, _framebuffers[flightFrame]);
    // Bind pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline->pipeline);
    // Set parameters buffer
    _parameters.aoSamples = _settings->occlusionSettings.numSamples;
    _parametersBuffer->copyData(&_parameters, sizeof(VolumeParameters));
    Light light = {};
    light.intensity = _settings->lightSettings.intensity;
    light.direction = _settings->lightSettings.direction;
    light.color = _settings->lightSettings.color;
    _scene->lightBuffer->copyData(&light, sizeof(Light));
    _pipeline->descriptorSet->writeImage(3, flightFrame, _positionTargets[altFrame].imageView, _positionTargets[altFrame].sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    _pipeline->descriptorSet->writeBuffer(4, flightFrame, _parametersBuffer->buffer, sizeof(VolumeParameters), vk::DescriptorType::eUniformBuffer);
    _pipeline->descriptorSet->writeBuffer(5, flightFrame, _scene->lightBuffer->buffer, _scene->lightBuffer->size, vk::DescriptorType::eUniformBuffer);
    // Bind descriptor sets
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline->layout,
        0, 1,
        _pipeline->descriptorSet->getSet(flightFrame),
        0, nullptr);
    cmd.draw(3, 1, 0, 0);
    // End color renderpass
    cmd.endRenderPass();

    cmdutil::imageMemoryBarrier(
        cmd,
        _colorTarget->image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    return {
        std::cref(*_colorTarget),
        std::cref(*_depthTarget),
        std::cref(*_motionTarget),
        std::cref(*_maskTarget),
        std::cref(*_normalTarget),
        std::cref(_positionTargets[flightFrame])
    };
}

const vk::PipelineLayout& GeometryStage::getPipelineLayout() const
{
    return _pipeline->layout;
}
