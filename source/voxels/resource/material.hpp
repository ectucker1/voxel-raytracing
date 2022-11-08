#pragma once

#include <glm/glm.hpp>

struct Material
{
    glm::vec4 diffuse;
    float metallic;
    float pad1;
    float pad2;
    float pad3;
};
