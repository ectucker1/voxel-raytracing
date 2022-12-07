#pragma once

#include <glm/glm.hpp>

struct VolumeParameters
{
    uint32_t aoSamples = 4;
    float ambientIntensity = 1.0f;
};

struct BlitOffsets
{
    glm::uvec2 sourceSize;
    glm::uvec2 targetSize;
};
