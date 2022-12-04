#include "upscaler_stage.hpp"

#include <vk/ffx_fsr2_vk.h>
#include "engine/engine.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/commands/command_util.hpp"

UpscalerStage::UpscalerStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings) : AVoxelRenderStage(engine, settings)
{
    engine->recreationQueue->push(RecreationEventFlags::TARGET_RESIZE, [&]() {
        _fsrInterface = static_cast<FfxFsr2Interface*>(malloc(sizeof(FfxFsr2Interface)));
        _fsrContext = static_cast<FfxFsr2Context*>(malloc(sizeof(FfxFsr2Context)));

        // Allocate scratch buffer
        size_t memorySize = ffxFsr2GetScratchMemorySizeVK(engine->physicalDevice);
        _fsrScratchBuffer = malloc(memorySize);

        // Get Vulkan interface for FSR
        FfxErrorCode errorCode = ffxFsr2GetInterfaceVK(_fsrInterface, _fsrScratchBuffer, memorySize, engine->physicalDevice, engine->getDeviceProcAddr);
        FFX_ASSERT(errorCode == FFX_OK);

        // Create FSR context
        FfxFsr2ContextDescription contextDesc = {};
        contextDesc.flags = 0;
        contextDesc.maxRenderSize = { settings->targetResolution.x, settings->targetResolution.y };
        contextDesc.displaySize = { settings->targetResolution.x, settings->targetResolution.y };
        contextDesc.device = engine->device;
        contextDesc.callbacks = *_fsrInterface;
        contextDesc.flags = FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_DEPTH_INFINITE;
        errorCode = ffxFsr2ContextCreate(_fsrContext, &contextDesc);
        FFX_ASSERT(errorCode == FFX_OK);

        // Queue deletors
        FfxFsr2Interface* localFsrInterface = _fsrInterface;
        FfxFsr2Context* localFsrContext = _fsrContext;
        void* localScratchBuffer = _fsrScratchBuffer;
        pushDeletor([=](const std::shared_ptr<Engine>&) {
            ffxFsr2ContextDestroy(localFsrContext);
            free(localScratchBuffer);
            free(localFsrContext);
            free(localFsrInterface);
        });

        return [=](const std::shared_ptr<Engine>&) {
            this->resetDestroy();
        };
    });

    engine->recreationQueue->push(RecreationEventFlags::RENDER_RESIZE | RecreationEventFlags::TARGET_RESIZE, [&]() {
        _target = std::make_unique<RenderImage>(engine, settings->targetResolution.x, settings->targetResolution.y, vk::Format::eR8G8B8A8Unorm,
                                     vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::ImageAspectFlagBits::eColor, "Upscaler Target");

        return [=](const std::shared_ptr<Engine>&) {
            _target->destroy();
        };
    });
}

void UpscalerStage::update(float delta)
{
    _deltaMsec = delta * 1000;

    const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(_settings->renderResolution().x, _settings->targetResolution.x);

    ffxFsr2GetJitterOffset(&jitterX, &jitterY, frameCount % jitterPhaseCount, jitterPhaseCount);

    frameCount++;
    if (frameCount > jitterPhaseCount)
        frameCount = 0;
}

FfxResource UpscalerStage::wrapRenderImage(const RenderImage& image)
{
    return ffxGetTextureResourceVK(_fsrContext, static_cast<VkImage>(image.image), static_cast<VkImageView>(image.imageView),
        image.width, image.height, static_cast<VkFormat>(image.format));
}

const RenderImage& UpscalerStage::record(const vk::CommandBuffer& cmd,
                                         const RenderImage& color, const RenderImage& depth, const RenderImage& motion, const RenderImage& mask)
{
    cmdutil::imageMemoryBarrier(
        cmd,
        depth.image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
        cmd,
        motion.image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
        cmd,
        mask.image,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    cmdutil::imageMemoryBarrier(
        cmd,
        _target->image,
        vk::AccessFlagBits::eNone,
        vk::AccessFlagBits::eMemoryRead,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::ImageAspectFlagBits::eColor);

    FfxFsr2DispatchDescription dispatchDescription = {};

    dispatchDescription.commandList = ffxGetCommandListVK(cmd);

    dispatchDescription.color = wrapRenderImage(color);
    dispatchDescription.depth = wrapRenderImage(depth);
    dispatchDescription.motionVectors = wrapRenderImage(motion);
    dispatchDescription.reactive = wrapRenderImage(mask);
    dispatchDescription.output = wrapRenderImage(*_target);

    dispatchDescription.jitterOffset.x = jitterX;
    dispatchDescription.jitterOffset.y = jitterY;

    dispatchDescription.motionVectorScale.x = 1.0;
    dispatchDescription.motionVectorScale.y = 1.0;

    dispatchDescription.reset = false;

    dispatchDescription.enableSharpening = true;
    dispatchDescription.sharpness = 1.0f;

    dispatchDescription.frameTimeDelta = _deltaMsec;

    dispatchDescription.preExposure = 1.0f;

    dispatchDescription.renderSize.width = _settings->renderResolution().x;
    dispatchDescription.renderSize.height = _settings->renderResolution().y;

    dispatchDescription.cameraFar = INFINITY;
    dispatchDescription.cameraNear = 0.0;
    dispatchDescription.cameraFovAngleVertical = 0.2f; // TODO calculate

    FfxErrorCode errorCode = ffxFsr2ContextDispatch(_fsrContext, &dispatchDescription);
    FFX_ASSERT(errorCode == FFX_OK);

    cmdutil::imageMemoryBarrier(
        cmd,
        _target->image,
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor);

    return *_target;
}
