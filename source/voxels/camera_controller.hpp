#pragma once

#include <glm/glm.hpp>

class GLFWwindow;

class CameraController
{
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 up;

private:
    float yaw;
    float pitch;
    glm::vec3 normalDir;

    float focalLength;

    double lastCursorX = -1;
    double lastCursorY = -1;

public:
    CameraController() = default;
    CameraController(glm::vec3 position, float yaw, float pitch, float focalLength);

    void update(GLFWwindow* window, float delta);
    void mouseCallback(GLFWwindow* window, double xpos, double ypos);

private:
    void updateDirectionVectors();
};
