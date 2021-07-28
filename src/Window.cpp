#include "Window.h"

lv::Window::Window() {
    if (!glfwInit()) throw std::runtime_error("Could not initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindow = glfwCreateWindow(640, 480, name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, onResizeCallback);
}

lv::Window::~Window() {
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

void lv::Window::onResizeCallback(GLFWwindow *glfwWindow, int width, int height) {
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->width = width;
    window->height = height;
    window->wasResized = true;
}