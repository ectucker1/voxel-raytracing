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
    FsrScaling scaling = FsrScaling::BALANCED;

    AmbientOcclusionSettings occlusionSettings = {};

public:
    glm::uvec2 renderResolution() const;
};

