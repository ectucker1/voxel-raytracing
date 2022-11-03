#pragma once

#include <memory>
#include "engine/recreation_queue.hpp"

class VoxelRenderSettings;
class VoxelRenderer;

namespace VoxelSettingsGui
{
    extern RecreationEventFlags draw(const std::shared_ptr<VoxelRenderSettings>& settings);
}
