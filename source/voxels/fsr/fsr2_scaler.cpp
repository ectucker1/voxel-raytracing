#include "fsr2_scaler.hpp"

#include <vk/ffx_fsr2_vk.h>
#include "engine/engine.hpp"
#include "engine/resource/render_image.hpp"

FSR2Scaler::FSR2Scaler(const std::shared_ptr<Engine>& engine, glm::uvec2 targetRes) : AResource(engine), targetRes(targetRes)
{
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
    contextDesc.maxRenderSize = { targetRes.x, targetRes.y };
    contextDesc.displaySize = { targetRes.x, targetRes.y };
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
}

void FSR2Scaler::update(float delta, glm::uvec2 renderRes)
{
    deltaMsec = delta * 1000;

    const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(renderRes.x, targetRes.x);

    ffxFsr2GetJitterOffset(&jitterX, &jitterY, frameCount % jitterPhaseCount, jitterPhaseCount);

    frameCount++;
    if (frameCount > jitterPhaseCount)
        frameCount = 0;
}

FfxResource FSR2Scaler::wrapRenderImage(const RenderImage& image)
{
    return ffxGetTextureResourceVK(_fsrContext, static_cast<VkImage>(image.image), static_cast<VkImageView>(image.imageView),
                                   image.width, image.height, static_cast<VkFormat>(image.format));
}

void FSR2Scaler::dispatch(const vk::CommandBuffer& cmd,
                          const RenderImage& color, const RenderImage& depth, const RenderImage& motion, const RenderImage& mask,
                          glm::uvec2 renderRes, const RenderImage& output)
{
    FfxFsr2DispatchDescription dispatchDescription = {};

    dispatchDescription.commandList = ffxGetCommandListVK(cmd);

    dispatchDescription.color = wrapRenderImage(color);
    dispatchDescription.depth = wrapRenderImage(depth);
    dispatchDescription.motionVectors = wrapRenderImage(motion);
    dispatchDescription.reactive = wrapRenderImage(mask);
    dispatchDescription.output = wrapRenderImage(output);

    dispatchDescription.jitterOffset.x = jitterX;
    dispatchDescription.jitterOffset.y = jitterY;

    dispatchDescription.motionVectorScale.x = 1.0;
    dispatchDescription.motionVectorScale.y = 1.0;

    dispatchDescription.reset = false;

    dispatchDescription.enableSharpening = true;
    dispatchDescription.sharpness = 1.0f;

    dispatchDescription.frameTimeDelta = deltaMsec;

    dispatchDescription.preExposure = 1.0f;

    dispatchDescription.renderSize.width = renderRes.x;
    dispatchDescription.renderSize.height = renderRes.y;

    dispatchDescription.cameraFar = INFINITY;
    dispatchDescription.cameraNear = 0.0;
    dispatchDescription.cameraFovAngleVertical = 0.2f; // TODO calculate

    FfxErrorCode errorCode = ffxFsr2ContextDispatch(_fsrContext, &dispatchDescription);
    FFX_ASSERT(errorCode == FFX_OK);
}
