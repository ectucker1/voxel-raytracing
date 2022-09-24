#pragma once

#include <glm/glm.hpp>

struct ScreenQuadPush
{
    glm::vec4 camPos;
    glm::vec4 camDir;
    glm::uvec3 volumeBounds;
    glm::float32 time;
    glm::ivec2 screenSize;
};
