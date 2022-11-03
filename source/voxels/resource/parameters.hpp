#pragma once

#include <glm/glm.hpp>

struct VolumeParameters
{
    uint32_t aoSamples = 4;
};

struct BlitOffsets
{
    glm::uvec2 sourceSize;
    glm::uvec2 targetSize;
};
