#pragma once

#include <memory>

class VoxelRenderSettings;

namespace VoxelSettingsGui
{
    extern void draw(const std::shared_ptr<VoxelRenderSettings>& settings);
}
