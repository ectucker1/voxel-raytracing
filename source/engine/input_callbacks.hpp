#pragma once

#include <vector>
#include <functional>

struct GLFWwindow;
struct GLFWmonitor;

class InputCallbacks
{
public:
    std::vector<std::function<void(GLFWwindow* window, int entered)>> cursorEnterCallbacks;
    std::vector<std::function<void(GLFWwindow* window, double xpos, double ypos)>> cursorPosCallbacks;
    std::vector<std::function<void(GLFWwindow* window, int button, int action, int mods)>> mouseButtonCallbacks;
    std::vector<std::function<void(GLFWwindow* window, double xoffset, double yoffset)>> scrollCallbacks;
    std::vector<std::function<void(GLFWwindow* window, unsigned int codepoint)>> charCallbacks;
    std::vector<std::function<void(GLFWwindow* window, int key, int scancode, int action, int mods)>> keyCallbacks;
    std::vector<std::function<void(GLFWwindow* window, int focused)>> windowFocusCallbacks;
};

namespace Inputs
{
    void registerCallbacks(GLFWwindow* window);
    void unregisterCallbacks(GLFWwindow* window);
}
