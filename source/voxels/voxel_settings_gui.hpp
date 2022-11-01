#pragma once

#include <memory>
#include "engine/recreation_queue.hpp"

class VoxelRenderSettings;
class VoxelSDFRenderer;

namespace VoxelSettingsGui
{
    extern RecreationEventFlags draw(const VoxelSDFRenderer& renderer, const std::shared_ptr<VoxelRenderSettings>& settings);
}
