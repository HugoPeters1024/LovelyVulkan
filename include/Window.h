#pragma once
#include "precomp.h"
#include "Structs.hpp"
#include "Device.h"
#include "Swapchain.h"


namespace lv {

// forward declaration for mutual dependency
class Device;

class Window : NoCopy {
private:
    Device& device;
    std::unique_ptr<Swapchain> swapchain;

    void createSurface();
    void recreateSwapchain();
    static void onResizeCallback(GLFWwindow* glfwWindow, int width, int height);

    int width, height;
    std::string name;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR vkSurface;
    bool wasResized = false;

public:
    Window(Device& device, const std::string &name, int width, int height);
    ~Window();

    bool shouldClose() const { return glfwWindowShouldClose(glfwWindow); }

    int32_t getWidth() const { return width; }
    int32_t getHeight() const { return height; }
    const std::string &getName() const { return name; }
    GLFWwindow *getGlfwWindow() const { return glfwWindow; }
    const VkSurfaceKHR getVkSurface() const { return vkSurface; }
    inline uint32_t getNrImages() const { return swapchain->nrImages(); }
    inline AttachmentView getColorAttachmentView(uint32_t binding) const { return swapchain->getColorAttachmentView(binding); }


    uint32_t startFrame();
    void submitFrame(const std::vector<VkCommandBuffer>& cmdBuffers, uint32_t imageIdx);
};

}