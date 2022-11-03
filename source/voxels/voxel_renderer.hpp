#include "engine/renderer.hpp"

#include <optional>
#include "voxels/resource/camera_controller.hpp"
#include "voxels/resource/voxel_scene.hpp"
#include "voxel_render_settings.hpp"

class GeometryStage;
class BlitStage;
class DenoiserStage;
class UpscalerStage;
class ImguiRenderer;
class Texture2D;

class VoxelRenderer : public ARenderer
{
private:
    std::unique_ptr<CameraController> _camera;

    std::shared_ptr<VoxelRenderSettings> _settings;

    std::shared_ptr<VoxelScene> _scene;
    std::shared_ptr<Texture2D> _noiseTexture;

    std::unique_ptr<GeometryStage> _geometryStage;
    std::unique_ptr<DenoiserStage> _denoiserStage;
    std::unique_ptr<UpscalerStage> _upscalerStage;
    std::unique_ptr<BlitStage> _blitStage;

    std::unique_ptr<ImguiRenderer> _imguiRenderer;

    float _time = 0;

public:
    VoxelRenderer(const std::shared_ptr<Engine>& engine);
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) override;
};
