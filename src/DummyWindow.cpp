#include "DummyWindow.h"

namespace lv {

DummyWindow::DummyWindow(VkInstance instance) : instance(instance) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindow = glfwCreateWindow(1, 1, "ignore", nullptr, nullptr);
    assert(glfwWindow && "Failed to create window");
    vkCheck(glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface));
}

DummyWindow::~DummyWindow() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwDestroyWindow(glfwWindow);
}

void DummyWindow::collectInstanceExtensions(std::set<std::string>& extensions) {
    uint32_t extensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    for(uint32_t i=0; i<extensionCount; i++) {
        extensions.insert(glfwExtensions[i]);
    }
}

}
