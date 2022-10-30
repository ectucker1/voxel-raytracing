#pragma once

#include <optional>
#include "util/resource_ring.hpp"
#include "engine/resource.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/pipeline/descriptor_set.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "voxels/pipeline/denoiser_pipeline.hpp"

#define MAX_DENOISER_PASSES 10

class VoxelRenderSettings;

class BlurDenoiser : public AResource
{
private:
    // Target resolution
    glm::uvec2 renderRes;

    // Common resources
    std::optional<Buffer> _denoiseKernelBuffer;
    std::optional<Buffer> _denoiseOffsetBuffer;

    // Per-iteration resources
    std::vector<Buffer> _denoiseParamsBuffer;
    std::vector<DescriptorSet> _denoiseDescriptors;

    // Ping-pong buffers
    ResourceRing<RenderImage> _denoiseColorTargets;
    ResourceRing<RenderPass> _denoisePasses;
    ResourceRing<Framebuffer> _denoiseFramebuffers;

    // Render pipeline
    std::optional<DenoiserPipeline> _denoisePipeline;

public:
    BlurDenoiser(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings, glm::uvec2 renderRes, float phiColor0, float phiNormal0, float phiPos0, float stepWidth);

    void setParameters(float phiColor0, float phiNormal0, float phiPos0, float stepWidth) const;

    const RenderImage& render(const vk::CommandBuffer& cmd, uint32_t flightFrame, int iterations,
                              const RenderImage& colorInput, const RenderImage& normalInput, const RenderImage& posInput) const;

private:
    void initRenderResResources();
};
