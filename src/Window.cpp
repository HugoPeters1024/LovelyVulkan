#include "Window.h"
#include "Device.h"

namespace lv {

Window::Window(Device& device, const std::string &name, int width, int height) : device(device), name(name), width(width), height(height) {
    if (!glfwInit()) throw std::runtime_error("Could not initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, onResizeCallback);

    createSurface();

    logger::info("glfw window created: {} {}x{}", name, width, height);
}

Window::~Window() {
    logger::info("Closing window {}", name);
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

void Window::createSurface() {
    device.getVkInstance();
}

void Window::onResizeCallback(GLFWwindow *glfwWindow, int width, int height) {
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->width = width;
    window->height = height;
    window->wasResized = true;
}

}