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

class VoxelSDFRenderer : public ARenderer
{
private:
    CameraController camera;

    glm::uvec2 renderRes = { 1920, 1080 };

    std::optional<RenderImage> gColorTarget;
    std::optional<RenderImage> gDepthTarget;
    std::optional<RenderPass> gPass;
    std::optional<Framebuffer> gFramebuffer;

    std::optional<RenderImage> denoiseColorTarget;
    std::optional<RenderPass> denoisePass;
    std::optional<Framebuffer> denoiseFramebuffer;

    std::optional<VoxelSDFPipeline> geometryPipeline;
    std::optional<DenoiserPipeline> denoisePipeline;
    std::optional<BlitPipeline> blitPipeline;

    std::optional<Texture3D> _sceneTexture;
    std::optional<Texture2D> _noiseTexture;
    std::optional<Buffer> _paletteBuffer;

    BlitOffsets blitOffsets;
    std::optional<Buffer> _blitOffsetsBuffer;

    float _time = 0;

public:
    VoxelSDFRenderer(const std::shared_ptr<Engine>& engine);
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) override;
};
