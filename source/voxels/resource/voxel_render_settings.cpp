#include "voxel_render_settings.hpp"

static uint32_t scale(FsrScaling scaling, uint32_t dim)
{
    return static_cast<uint32_t>((10.0f / static_cast<uint32_t>(scaling)) * dim);
}

glm::uvec2 VoxelRenderSettings::renderResolution() const
{
    return glm::uvec2(scale(scaling, targetResolution.x), scale(scaling, targetResolution.y));
}
