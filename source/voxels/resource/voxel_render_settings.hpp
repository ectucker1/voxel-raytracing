#pragma once

#include "util/bidirectional_event_queue.hpp"
#include <glm/glm.hpp>

enum class FsrScaling : uint32_t
{
    NONE = 10,
    QUALITY = 15,
    BALANCED = 17,
    PERFORMANCE = 20,
    ULTRA_PERFORMANCE = 30
};

struct FsrSettings
{
    bool enable = true;
    FsrScaling scaling = FsrScaling::BALANCED;
};

struct DenoiserSettings
{
    bool enable = true;
    int iterations = 3;
    float phiColor0 = 20.4f;
    float phiNormal0 = 1E-2f;
    float phiPos0 = 1E-1f;
    float stepWidth = 2.0f;
};

struct AmbientOcclusionSettings
{
    uint32_t numSamples = 4;
};

class VoxelRenderSettings
{
public:
    BidirectionalEventQueue targetResListeners;
    BidirectionalEventQueue renderResListeners;

public:
    glm::uvec2 targetResolution = { 3840, 2160 };

    FsrSettings fsrSetttings = {};
    DenoiserSettings denoiserSettings = {};
    AmbientOcclusionSettings occlusionSettings = {};

public:
    glm::uvec2 renderResolution() const;
};

