#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <ffx_fsr2.h>
#include "voxels/voxel_render_stage.hpp"

class RenderImage;

class UpscalerStage : public AVoxelRenderStage
{
public:
    float jitterX;
    float jitterY;
    int frameCount;

private:
    std::unique_ptr<RenderImage> _target;

    FfxFsr2Interface* _fsrInterface;
    FfxFsr2Context* _fsrContext;
    void* _fsrScratchBuffer;

    float _deltaMsec;

public:
    UpscalerStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings);

    void update(float delta);
    const RenderImage& record(const vk::CommandBuffer& cmd,
        const RenderImage& color, const RenderImage& depth,
        const RenderImage& motion, const RenderImage& mask);

private:
    FfxResource wrapRenderImage(const RenderImage& image);
};
