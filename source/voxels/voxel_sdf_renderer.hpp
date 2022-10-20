#include "engine/renderer.hpp"

#include <optional>
#include "util/resource_ring.hpp"
#include "voxel_sdf_pipeline.hpp"
#include "denoiser_pipeline.hpp"
#include "blit_pipeline.hpp"
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/texture_2d.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/resource/render_image.hpp"
#include "engine/pipeline/render_pass.hpp"
#include "engine/pipeline/framebuffer.hpp"
#include "voxels/camera_controller.hpp"
#include "voxels/fsr/fsr2_scaler.hpp"
#include "voxels/voxel_scene.hpp"
#include "engine/gui/imgui_renderer.hpp"

class VoxelSDFRenderer : public ARenderer
{
private:
    std::unique_ptr<CameraController> camera;

    glm::uvec2 renderRes = { 1920, 1080 };
    glm::uvec2 upscaleRes = { 3840, 2160 };

    std::optional<RenderImage> gColorTarget;
    std::optional<RenderImage> gDepthTarget;
    std::optional<RenderImage> gMotionTarget;
    std::optional<RenderImage> gMaskTarget;
    ResourceRing<RenderImage> gPositionTargets;
    std::optional<RenderPass> gPass;
    ResourceRing<Framebuffer> gFramebuffers;

    std::optional<FSR2Scaler> upscaler;
    std::optional<RenderImage> upscalerTarget;

    std::optional<RenderImage> denoiseColorTarget;
    std::optional<RenderPass> denoisePass;
    std::optional<Framebuffer> denoiseFramebuffer;

    std::optional<VoxelSDFPipeline> geometryPipeline;
    std::optional<DenoiserPipeline> denoisePipeline;
    std::optional<BlitPipeline> blitPipeline;

    std::optional<VoxelScene> _scene;
    std::optional<Texture2D> _noiseTexture;

    BlitOffsets blitOffsets;
    std::optional<Buffer> _blitOffsetsBuffer;

    std::optional<ImguiRenderer> _imguiRenderer;

    float _time = 0;

public:
    VoxelSDFRenderer(const std::shared_ptr<Engine>& engine);
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) override;
};
