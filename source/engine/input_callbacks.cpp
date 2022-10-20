#include "input_callbacks.hpp"

#include <GLFW/glfw3.h>
#include "engine/engine.hpp"

static void cursorEnterCallback(GLFWwindow* window, int entered)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.cursorEnterCallbacks)
        callback(window, entered);
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.cursorPosCallbacks)
        callback(window, xpos, ypos);
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.mouseButtonCallbacks)
        callback(window, button, action, mods);
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.scrollCallbacks)
        callback(window, xoffset, yoffset);
}

static void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.charCallbacks)
        callback(window, codepoint);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.keyCallbacks)
        callback(window, key, scancode, action, mods);
}

static void windowFocusCallback(GLFWwindow* window, int focused)
{
    Engine* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    for (auto callback : engine->inputs.windowFocusCallbacks)
        callback(window, focused);
}

void Inputs::registerCallbacks(GLFWwindow* window)
{
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowFocusCallback(window, windowFocusCallback);
}

void Inputs::unregisterCallbacks(GLFWwindow* window)
{
    glfwSetCursorEnterCallback(window, nullptr);
    glfwSetCursorPosCallback(window, nullptr);
    glfwSetMouseButtonCallback(window, nullptr);
    glfwSetScrollCallback(window, nullptr);
    glfwSetCharCallback(window, nullptr);
    glfwSetKeyCallback(window, nullptr);
    glfwSetWindowFocusCallback(window, nullptr);
}
