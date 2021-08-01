#include "Window.h"
#include "Device.h"

namespace lv {

Window::Window(Device& device, const std::string &name, int width, int height) : device(device), name(name), width(width), height(height) {
    if (!glfwInit()) throw std::runtime_error("Could not initialize GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(glfwWindow, this);
    glfwSetWindowSizeCallback(glfwWindow, onResizeCallback);

    createSurface();
    swapchain = std::make_unique<Swapchain>(*this, device);

    logger::info("glfw window created: {} {}x{}", name, width, height);
}

Window::~Window() {
    logger::info("Closing window {}", name);
    swapchain.reset();
    vkDestroySurfaceKHR(device.getVkInstance(), vkSurface, nullptr);
    glfwDestroyWindow(glfwWindow);
}

void Window::createSurface() {
    vkCheck(glfwCreateWindowSurface(device.getVkInstance(), glfwWindow, nullptr, &vkSurface));
}

void Window::recreateSwapchain() {
    vkCheck(vkDeviceWaitIdle(device.getVkDevice()));
    swapchain = std::make_unique<Swapchain>(*this, device, std::move(swapchain));
}

void Window::onResizeCallback(GLFWwindow *glfwWindow, int width, int height) {
    auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->width = width;
    window->height = height;
    window->wasResized = true;
}

uint32_t Window::startFrame() {
    uint32_t imageIdx;
    auto result = swapchain->startFrame(&imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return startFrame();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        logger::error("Failed to acquire swapchain image");
        exit(1);
    }

    return imageIdx;
}

void Window::submitFrame(const std::vector<VkCommandBuffer>& cmdBuffers, uint32_t imageIdx) {
    swapchain->submitFrame(cmdBuffers, imageIdx);
    glfwPollEvents();
}

}