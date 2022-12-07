#pragma once

#include "engine/recreation_queue.hpp"
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
    int iterations = 2;
    float phiColor0 = 20.4f;
    float phiNormal0 = 1E-2f;
    float phiPos0 = 1E-1f;
    float stepWidth = 2.0f;
};

struct AmbientOcclusionSettings
{
    int numSamples = 4;
    float intensity = 1.0f;
};

struct LightSettings
{
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
};

class VoxelRenderSettings
{
public:
    glm::uvec2 targetResolution = { 1920, 1080 };

    FsrSettings fsrSetttings = {};
    DenoiserSettings denoiserSettings = {};
    AmbientOcclusionSettings occlusionSettings = {};
    LightSettings lightSettings = {};

public:
    glm::uvec2 renderResolution() const;
};

