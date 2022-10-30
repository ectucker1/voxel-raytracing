#include "engine/renderer.hpp"

#include <optional>
#include "util/resource_ring.hpp"
#include "voxels/pipeline/voxel_sdf_pipeline.hpp"
#include "voxels/pipeline/denoiser_pipeline.hpp"
#include "voxels/pipeline/blit_pipeline.hpp"
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/texture_2d.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "voxels/resource/camera_controller.hpp"
#include "voxels/fsr/fsr2_scaler.hpp"
#include "voxels/resource/voxel_scene.hpp"
#include "engine/gui/imgui_renderer.hpp"
#include "voxels/resource/voxel_render_settings.hpp"
#include "voxels/blur_denoiser.hpp"
#include "voxels/resource/parameters.hpp"

class VoxelSDFRenderer : public ARenderer
{
public:
    std::unique_ptr<BlurDenoiser> denoiser;

private:
    std::unique_ptr<CameraController> camera;

    std::shared_ptr<VoxelRenderSettings> settings;

    std::optional<RenderImage> gColorTarget;
    std::optional<RenderImage> gDepthTarget;
    std::optional<RenderImage> gMotionTarget;
    std::optional<RenderImage> gMaskTarget;
    ResourceRing<RenderImage> gPositionTargets;
    std::optional<RenderImage> gNormalTarget;
    std::optional<RenderPass> gPass;
    ResourceRing<Framebuffer> gFramebuffers;

    std::optional<FSR2Scaler> upscaler;
    std::optional<RenderImage> upscalerTarget;

    std::optional<VoxelSDFPipeline> geometryPipeline;
    std::optional<BlitPipeline> blitPipeline;

    std::optional<VoxelScene> _scene;
    std::optional<Texture2D> _noiseTexture;

    VolumeParameters volumeParameters;
    std::optional<Buffer> _volumeParametersBuffer;

    BlitOffsets blitOffsets;
    std::optional<Buffer> _blitOffsetsBuffer;

    std::optional<ImguiRenderer> _imguiRenderer;

    float _time = 0;

public:
    VoxelSDFRenderer(const std::shared_ptr<Engine>& engine);
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) override;

private:
    void initRenderTargets();
    void initRenderPasses();
    void initFramebuffers();
    void initRenderPipelines();
    void initRenderPipelineTargets();
    void initUpscaler();
    void initBlitPipelines();
    void initBlitPipelineTargets();
};
