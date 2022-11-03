#include "voxel_render_settings.hpp"

static uint32_t scale(FsrScaling scaling, uint32_t dim)
{
    return static_cast<uint32_t>((10.0f / static_cast<uint32_t>(scaling)) * dim);
}

glm::uvec2 VoxelRenderSettings::renderResolution() const
{
    if (fsrSetttings.enable)
        return glm::uvec2(scale(fsrSetttings.scaling, targetResolution.x), scale(fsrSetttings.scaling, targetResolution.y));
    return targetResolution;
}
