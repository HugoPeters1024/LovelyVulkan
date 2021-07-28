#pragma once
#include "precomp.h"


namespace lv {

// forward declaration for mutual dependency
class Device;

class Window : NoCopy {
private:
    Device& device;

    void createSurface();
    static void onResizeCallback(GLFWwindow* glfwWindow, int width, int height);

    int width, height;
    std::string name;
    GLFWwindow* glfwWindow;
    bool wasResized = false;
public:
    Window(Device& device, const std::string &name, int width, int height);
    ~Window();

    bool shouldClose() const { return glfwWindowShouldClose(glfwWindow); }
    void processEvents() const { glfwPollEvents(); }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const std::string &getName() const { return name; }
    GLFWwindow *getGlfwWindow() const { return glfwWindow; } };

}