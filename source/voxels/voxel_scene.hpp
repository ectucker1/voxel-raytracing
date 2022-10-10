#pragma once

#include <string>
#include <optional>
#include "engine/resource/texture_3d.hpp"
#include "engine/resource/buffer.hpp"

// A complete voxel scene, including the 3D scene texture and palette
class VoxelScene : AResource
{
public:
    uint32_t width, height, depth;
    // The 3D scene texture
    std::optional<Texture3D> sceneTexture;
    // The buffer holding the material palette
    std::optional<Buffer> paletteBuffer;

public:
    // Loads a new voxel scene from the given file
    VoxelScene(const std::shared_ptr<Engine>& engine, const std::string& filename);
};
