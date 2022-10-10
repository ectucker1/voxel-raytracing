#pragma once

#include <glm/glm.hpp>

struct ScreenQuadPush
{
    glm::vec4 camPos;
    glm::vec4 camDir;
    glm::vec4 camRight;
    glm::vec4 camUp;
    glm::uvec3 volumeBounds;
    glm::uvec1 frame;
    glm::ivec2 screenSize;
    glm::vec2 cameraJitter;
};
