#pragma once

#include <optional>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "util/resource_ring.hpp"
#include "voxels/voxel_render_stage.hpp"

#define MAX_DENOISER_PASSES 10
#define DENOISER_KERNEL_RADIUS 1
#define DENOISER_KERNEL_SIZE ((DENOISER_KERNEL_RADIUS + DENOISER_KERNEL_RADIUS + 1) * (DENOISER_KERNEL_RADIUS + DENOISER_KERNEL_RADIUS + 1))

class VoxelRenderSettings;
class DenoiserPipeline;
class Buffer;
class DescriptorSet;
class RenderImage;
class RenderPass;
class Framebuffer;

class DenoiserStage : public AVoxelRenderStage
{
private:
    // Common resources
    std::unique_ptr<Buffer> _kernelBuffer;
    std::unique_ptr<Buffer> _offsetBuffer;

    // Per-iteration resources
    std::vector<Buffer> _iterationParamsBuffers;
    std::vector<DescriptorSet> _iterationDescriptors;

    // Ping-pong buffers
    ResourceRing<RenderImage> _colorTargets;
    ResourceRing<RenderPass> _renderPasses;
    ResourceRing<Framebuffer> _framebuffers;

    // Render pipeline
    std::unique_ptr<DenoiserPipeline> _pipeline;

public:
    DenoiserStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings);

    void updateParameters() const;

    const RenderImage& record(const vk::CommandBuffer& cmd, uint32_t flightFrame,
                              const RenderImage& colorInput, const RenderImage& normalInput, const RenderImage& posInput) const;
};
