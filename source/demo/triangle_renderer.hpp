#include "engine/renderer.hpp"
#include "util/resource_ring.hpp"
#include "demo/triangle_pipeline.hpp"

class TriangleRenderer : public ARenderer
{
private:
    std::unique_ptr<TrianglePipeline> _pipeline;

    float _time = 0;

public:
    TriangleRenderer(const std::shared_ptr<Engine>& engine);
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t swapchainImage, uint32_t flightFrame) override;
};
