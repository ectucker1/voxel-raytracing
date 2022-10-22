#pragma once

#include <ffx_fsr2.h>
#include "engine/resource.hpp"
#include "engine/resource/buffer.hpp"
#include "engine/resource/render_image.hpp"

class FSR2Scaler : public AResource
{
public:
    float jitterX;
    float jitterY;
    int frameCount;

private:
    glm::uvec2 targetRes;

    FfxFsr2Interface* _fsrInterface;
    FfxFsr2Context* _fsrContext;
    void* _fsrScratchBuffer;

    float deltaMsec;

public:
    FSR2Scaler(const std::shared_ptr<Engine>& engine, glm::uvec2 targetRes);

    void update(float delta, glm::uvec2 renderRes);
    void dispatch(const vk::CommandBuffer& cmd,
                  const RenderImage& color, const RenderImage& depth,
                  const RenderImage& motion, const RenderImage& mask,
                  glm::uvec2 renderRes, const RenderImage& output);

private:
    FfxResource wrapRenderImage(const RenderImage& image);
};
