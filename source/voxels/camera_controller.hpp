#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;
class Engine;

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

    std::shared_ptr<Engine> engine;

public:
    CameraController() = default;
    CameraController(const std::shared_ptr<Engine>& engine, glm::vec3 position, float yaw, float pitch, float focalLength);

    void update(float delta);
    void mouseCallback(GLFWwindow* window, double xpos, double ypos);

private:
    void updateDirectionVectors();
};
