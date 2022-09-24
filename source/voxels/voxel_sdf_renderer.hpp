#include "engine/renderer.hpp"
#include "util/resource_ring.hpp"
#include "voxel_sdf_pipeline.hpp"
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/render_image.hpp"

class VoxelSDFRenderer : public ARenderer
{
private:
    glm::uvec2 renderRes = { 1920, 1080 };
    ResourceRing<RenderImage> _renderColorTarget;
    vk::RenderPass _renderColorPass;
    ResourceRing<vk::Framebuffer> _renderColorFramebuffer;

    std::shared_ptr<Texture3D> _sceneTexture;
    VoxelSDFPipeline _pipeline;

    float _time = 0;

public:
    virtual void init(const std::shared_ptr<Engine>& engine) override;
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t flightFrame) override;
};
