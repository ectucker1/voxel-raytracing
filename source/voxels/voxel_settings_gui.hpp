#pragma once

#include <memory>

class VoxelRenderSettings;
class VoxelSDFRenderer;

namespace VoxelSettingsGui
{
    extern void draw(const VoxelSDFRenderer& renderer, const std::shared_ptr<VoxelRenderSettings>& settings);
}
