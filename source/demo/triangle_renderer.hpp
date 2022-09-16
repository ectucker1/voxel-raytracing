#include "engine/renderer.hpp"
#include "util/resource_ring.hpp"
#include "demo/triangle_pipeline.hpp"

class TriangleRenderer : public ARenderer
{
private:
    TrianglePipeline _pipeline;

    float _time = 0;

public:
    virtual void init(const std::shared_ptr<Engine>& engine) override;
    virtual void update(float delta) override;
    virtual void recordCommands(const vk::CommandBuffer& commandBuffer, uint32_t flightFrame) override;
};
