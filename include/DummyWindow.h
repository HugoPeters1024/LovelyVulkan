#pragma once
#include "precomp.h"

namespace lv {

class DummyWindow : NoCopy {
private:
    VkInstance instance;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR surface;
public:
    DummyWindow(VkInstance instance);
    ~DummyWindow();

    static void collectInstanceExtensions(std::set<std::string>& extensions);

    GLFWwindow *getGlfwWindow() const { return glfwWindow; }
    const VkSurfaceKHR &getSurface() const { return surface; }
};

}
