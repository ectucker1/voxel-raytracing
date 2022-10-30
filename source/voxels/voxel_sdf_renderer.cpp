#include "voxel_sdf_renderer.hpp"

#include "engine/engine.hpp"
#include "engine/commands/command_util.hpp"
#include "voxels/resource/screen_quad_push.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/resource/texture_2d.hpp"
#include <voxels/voxel_settings_gui.hpp>
#include <glm/gtx/functions.hpp>

VoxelSDFRenderer::VoxelSDFRenderer(const std::shared_ptr<Engine>& engine) : ARenderer(engine)
{
    settings = std::make_shared<VoxelRenderSettings>();

    camera = std::make_unique<CameraController>(engine, glm::vec3(8, 8, -50), 90.0f, 0.0f, static_cast<float>(1 / glm::tan(glm::radians(55.0f / 2))));

    _scene = VoxelScene(engine, "../resource/treehouse.vox");
    _noiseTexture = Texture2D(engine, "../resource/blue_noise_rgba.png", 4, vk::Format::eR8G8B8A8Unorm);

    _blitOffsetsBuffer = Buffer(engine, sizeof(BlitOffsets), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _blitOffsetsBuffer->copyData(&blitOffsets, sizeof(BlitOffsets));

    _volumeParametersBuffer = Buffer(engine, sizeof(VolumeParameters), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _volumeParametersBuffer->copyData(&volumeParameters, sizeof(VolumeParameters));

    initRenderTargets();
    initRenderPasses();
    initFramebuffers();
    initRenderPipelines();
    initUpscaler();
    initBlitPipelines();

    _imguiRenderer = ImguiRenderer(engine, _windowRenderPass->renderPass);
}

void VoxelSDFRenderer::initRenderTargets()
{
    glm::uvec2  renderRes = settings->renderResolution();

    gColorTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Unorm,
                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gDepthTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR32Sfloat,
                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gMotionTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR32G32Sfloat,
                                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gMaskTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8Unorm,
                              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gPositionTargets = ResourceRing<RenderImage>::fromArgs(2, engine, renderRes.x, renderRes.y, vk::Format::eR32G32B32A32Sfloat,
                                                           vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);
    gNormalTarget = RenderImage(engine, renderRes.x, renderRes.y, vk::Format::eR8G8B8A8Snorm,
                              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);

    denoiser = std::make_unique<BlurDenoiser>(engine, settings, renderRes,
                                              settings->denoiserSettings.phiColor0,
                                              settings->denoiserSettings.phiNormal0,
                                              settings->denoiserSettings.phiPos0,
                                              settings->denoiserSettings.stepWidth);

    settings->renderResListeners.push([&]() {
        engine->device.waitIdle();
        gColorTarget->destroy();
        gDepthTarget->destroy();
        gMotionTarget->destroy();
        gMaskTarget->destroy();
        gPositionTargets.destroy([&](RenderImage image) {
            image.destroy();
        });
        gNormalTarget->destroy();
        denoiser->destroy();
    },
                                      [&]() {
        initRenderTargets();
    });
}

void VoxelSDFRenderer::initRenderPasses()
{
    gPass = RenderPassBuilder(engine)
        .color(0, gColorTarget->format, glm::vec4(0.0))
        .color(1, gDepthTarget->format, glm::vec4(0.0))
        .color(2, gMotionTarget->format, glm::vec4(0.0))
        .color(3, gMaskTarget->format, glm::vec4(0.0))
        .color(4, gPositionTargets[0].format, glm::vec4(0.0))
        .color(5, gNormalTarget->format, glm::vec4(0.0))
        .build();

    settings->renderResListeners.push([&]() {
        engine->device.waitIdle();
        gPass->destroy();
    },
                                      [&]() {
        initRenderPasses();
    });
}

void VoxelSDFRenderer::initFramebuffers()
{
    gFramebuffers = ResourceRing<Framebuffer>::fromFunc(2, [&](uint32_t n) {
        return FramebufferBuilder(engine, gPass->renderPass, settings->renderResolution())
            .color(gColorTarget->imageView)
            .color(gDepthTarget->imageView)
            .color(gMotionTarget->imageView)
            .color(gMaskTarget->imageView)
            .color(gPositionTargets[n].imageView)
            .color(gNormalTarget->imageView)
            .build();
    });

    settings->renderResListeners.push([&]() {
        engine->device.waitIdle();
        gFramebuffers.destroy([&](Framebuffer framebuffer) {
            framebuffer.destroy();
        });
    },
                                      [&]() {
        initFramebuffers();
    });
}

void VoxelSDFRenderer::initRenderPipelines()
{
    geometryPipeline = VoxelSDFPipeline(VoxelSDFPipeline::build(engine, gPass->renderPass));

    initRenderPipelineTargets();
}

void VoxelSDFRenderer::initRenderPipelineTargets()
{
    geometryPipeline->descriptorSet->initImage(0, _scene->sceneTexture->imageView, _scene->sceneTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    geometryPipeline->descriptorSet->initBuffer(1, _scene->paletteBuffer->buffer, _scene->paletteBuffer->size, vk::DescriptorType::eUniformBuffer);
    geometryPipeline->descriptorSet->initImage(2, _noiseTexture->imageView, _noiseTexture->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    geometryPipeline->descriptorSet->initImage(3, gPositionTargets[0].imageView, gPositionTargets[0].sampler, vk::ImageLayout::eShaderReadOnlyOptimal);

    settings->renderResListeners.push([&]() {}, [&]() {
        initRenderPipelineTargets();
    });
}

void VoxelSDFRenderer::initUpscaler()
{
    upscaler = FSR2Scaler(engine, settings->targetResolution);
    upscalerTarget = RenderImage(engine, settings->targetResolution.x, settings->targetResolution.y, vk::Format::eR8G8B8A8Unorm,
                                 vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor);

    settings->targetResListeners.push([&]() {
        engine->device.waitIdle();
        upscaler->destroy();
        upscalerTarget->destroy();
    },
                                      [&]() {
        engine->device.waitIdle();
        initUpscaler();
    });
}

void VoxelSDFRenderer::initBlitPipelines()
{
    blitPipeline = BlitPipeline(BlitPipeline::build(engine, _windowRenderPass->renderPass));

    initBlitPipelineTargets();
}

void VoxelSDFRenderer::initBlitPipelineTargets()
{
    blitPipeline->descriptorSet->initImage(0, upscalerTarget->imageView, upscalerTarget->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    blitPipeline->descriptorSet->initBuffer(1, _blitOffsetsBuffer->buffer, _blitOffsetsBuffer->size, vk::DescriptorType::eUniformBuffer);

    /*settings->targetResListeners.push([&]() {}, [&]() {
        engine->device.waitIdle();
        initBlitPipelineTargets();
    });*/
}

void VoxelSDFRenderer::update(float delta)
{
    _time += delta;
    camera->update(delta);
    upscaler->update(delta, settings->renderResolution());
    _imguiRenderer->beginFrame();
    VoxelSettingsGui::draw(*this, settings);
}

void VoxelSDFRenderer::recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame)
{
    // Set viewport
    vk::Viewport renderResViewport;
    renderResViewport.x = 0.0f;
    renderResViewport.y = 0.0f;
    renderResViewport.width = static_cast<float>(settings->renderResolution().x);
    renderResViewport.height = static_cast<float>(settings->renderResolution().y);
    renderResViewport.minDepth = 0.0f;
    renderResViewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &renderResViewport);

    // Set scissor
    vk::Rect2D renderResScissor;
    renderResScissor.offset = vk::Offset2D(0, 0);
    renderResScissor.extent = vk::Extent2D(settings->renderResolution().x, settings->renderResolution().y);
    commandBuffer.setScissor(0, 1, &renderResScissor);

    uint32_t altFrame = (flightFrame + 1) % 2;
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gPositionTargets[flightFrame].image,
            vk::AccessFlagBits::eShaderRead,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor);
    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gPositionTargets[altFrame].image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);
    geometryPipeline->descriptorSet->writeImage(3, flightFrame, gPositionTargets[altFrame].imageView, gPositionTargets[altFrame].sampler, vk::ImageLayout::eShaderReadOnlyOptimal);

    // Start color renderpass
    gPass->recordBegin(commandBuffer, gFramebuffers[flightFrame]);
    // Bind pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPipeline->pipeline);
    // Set push constants
    ScreenQuadPush constants;
    constants.screenSize = glm::ivec2(settings->renderResolution().x, settings->renderResolution().y);
    constants.volumeBounds = glm::uvec3(_scene->width, _scene->height, _scene->depth);
    constants.camPos = glm::vec4(camera->position, 1);
    constants.camDir = glm::vec4(camera->direction, 0);
    constants.camUp = glm::vec4(camera->up, 0);
    constants.camRight = glm::vec4(camera->right, 0);
    constants.frame = glm::uvec1(upscaler->frameCount);
    constants.cameraJitter.x = upscaler->jitterX;
    constants.cameraJitter.y = upscaler->jitterY;
    commandBuffer.pushConstants(geometryPipeline->layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(ScreenQuadPush), &constants);
    // Set parameters buffer
    volumeParameters.aoSamples = settings->occlusionSettings.numSamples;
    _volumeParametersBuffer->copyData(&volumeParameters, sizeof(VolumeParameters));
    geometryPipeline->descriptorSet->writeBuffer(4, flightFrame, _volumeParametersBuffer->buffer, sizeof(VolumeParameters), vk::DescriptorType::eUniformBuffer);
    commandBuffer.setViewport(0, 1, &renderResViewport);
    // Set scissor
    commandBuffer.setScissor(0, 1, &renderResScissor);
    // Bind descriptor sets
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, geometryPipeline->layout,
                                     0, 1,
                                     geometryPipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);
    commandBuffer.draw(3, 1, 0, 0);
    // End color renderpass
    commandBuffer.endRenderPass();

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gColorTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gDepthTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gMotionTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gMaskTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gPositionTargets[flightFrame].image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            gNormalTarget->image,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::ImageAspectFlagBits::eColor);

    const RenderImage* denoiserOutput = &gColorTarget.value();
    if (settings->denoiserSettings.enable)
    {
        denoiserOutput = &denoiser->render(commandBuffer, flightFrame, settings->denoiserSettings.iterations, gColorTarget.value(), gNormalTarget.value(), gPositionTargets[flightFrame]);
    }

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            upscalerTarget->image,
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageAspectFlagBits::eColor);

    if (settings->fsrSetttings.enable)
    {
        upscaler->dispatch(commandBuffer, *denoiserOutput, gDepthTarget.value(),
                           gMotionTarget.value(), gMaskTarget.value(),
                           settings->renderResolution(), upscalerTarget.value());

        cmdutil::imageMemoryBarrier(
                commandBuffer,
                upscalerTarget->image,
                vk::AccessFlagBits::eMemoryWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::ImageAspectFlagBits::eColor);
    }

    // Set viewport
    vk::Viewport presentResViewport;
    presentResViewport.x = 0.0f;
    presentResViewport.y = 0.0f;
    presentResViewport.width = static_cast<float>(engine->windowSize.x);
    presentResViewport.height = static_cast<float>(engine->windowSize.y);
    presentResViewport.minDepth = 0.0f;
    presentResViewport.maxDepth = 1.0f;

    // Set scissor
    vk::Rect2D presentResScissor;
    presentResScissor.offset = vk::Offset2D(0, 0);
    presentResScissor.extent = vk::Extent2D(engine->windowSize.x, engine->windowSize.y);

    blitOffsets.sourceSize = { settings->targetResolution.x, settings->targetResolution.y };
    blitOffsets.targetSize = engine->windowSize;
    _blitOffsetsBuffer->copyData(&blitOffsets, sizeof(BlitOffsets));
    if (settings->fsrSetttings.enable)
    {
        blitPipeline->descriptorSet->writeImage(0, flightFrame, upscalerTarget->imageView, upscalerTarget->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
    else
    {
        blitPipeline->descriptorSet->writeImage(0,flightFrame, denoiserOutput->imageView, denoiserOutput->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
    blitPipeline->descriptorSet->writeBuffer(1, flightFrame, _blitOffsetsBuffer->buffer, sizeof(BlitOffsets), vk::DescriptorType::eUniformBuffer);

    // Copy color target into swapchain image
    _windowRenderPass->recordBegin(commandBuffer, _windowFramebuffers[swapchainImage]);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, blitPipeline->pipeline);
    commandBuffer.setViewport(0, 1, &presentResViewport);
    commandBuffer.setScissor(0, 1, &presentResScissor);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blitPipeline->layout,
                                     0, 1,
                                     blitPipeline->descriptorSet->getSet(flightFrame),
                                     0, nullptr);
    commandBuffer.draw(3, 1, 0, 0);
    _imguiRenderer->draw(commandBuffer);
    commandBuffer.endRenderPass();

    cmdutil::imageMemoryBarrier(
            commandBuffer,
            engine->swapchain.images[swapchainImage],
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eNone,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::ImageAspectFlagBits::eColor);
}
