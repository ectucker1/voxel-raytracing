#pragma once

#include <string>
#include <optional>
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/buffer.hpp"

struct Light
{
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    float intensity = 1.0f;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

// A complete voxel scene, including the 3D scene texture and palette
class VoxelScene : AResource
{
public:
    uint32_t width, height, depth;
    // The 3D scene texture
    std::optional<Texture3D> sceneTexture;
    // The buffer holding the material palette
    std::optional<Buffer> paletteBuffer;
    // The buffer holding the light
    std::optional<Buffer> lightBuffer;

public:
    // Loads a new voxel scene from the given file
    VoxelScene(const std::shared_ptr<Engine>& engine, const std::string& filename);
};
