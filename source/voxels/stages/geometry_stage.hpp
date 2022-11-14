#pragma once

#include <memory>
#include <array>
#include <vulkan/vulkan.hpp>
#include "voxels/voxel_render_stage.hpp"
#include "util/resource_ring.hpp"
#include "voxels/resource/parameters.hpp"

class RenderImage;
class RenderPass;
class Framebuffer;
class VoxelScene;
class Texture2D;
class Buffer;
class VoxelRenderSettings;
class VoxelSDFPipeline;

struct GeometryBuffer
{
    std::reference_wrapper<const RenderImage> color;
    std::reference_wrapper<const RenderImage> depth;
    std::reference_wrapper<const RenderImage> motion;
    std::reference_wrapper<const RenderImage> mask;
    std::reference_wrapper<const RenderImage> normal;
    std::reference_wrapper<const RenderImage> position;
};

class GeometryStage : public AVoxelRenderStage
{
private:
    std::shared_ptr<VoxelScene> _scene;

    VolumeParameters _parameters = {};
    std::unique_ptr<Buffer> _parametersBuffer;

    std::unique_ptr<RenderImage> _colorTarget;
    std::unique_ptr<RenderImage> _depthTarget;
    std::unique_ptr<RenderImage> _motionTarget;
    std::unique_ptr<RenderImage> _maskTarget;
    std::unique_ptr<RenderImage> _normalTarget;
    ResourceRing<RenderImage> _positionTargets;

    std::unique_ptr<RenderPass> _renderPass;

    ResourceRing<Framebuffer> _framebuffers;

    std::unique_ptr<VoxelSDFPipeline> _pipeline;

public:
    GeometryStage(const std::shared_ptr<Engine>& engine, const std::shared_ptr<VoxelRenderSettings>& settings, const std::shared_ptr<VoxelScene>& scene, const std::shared_ptr<Texture2D>& noise);

    GeometryBuffer record(const vk::CommandBuffer& cmd, uint32_t flightFrame);

    const vk::PipelineLayout& getPipelineLayout() const;
};
