#pragma once
#include "precomp.h"
#include "AppExt.h"
#include "FrameManager.h"


namespace lv {

class AppContext;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct WindowFrame : public FrameExt {
    VkImage vkImage;
    VkImageView vkView;
    VkFormat format;
    uint32_t width;
    uint32_t height;
};

struct WindowInfo {
    std::string windowName;
    uint32_t width;
    uint32_t height;
};


class Window : public FrameManager, NoCopy {
public:
    Window(AppContext& ctx, WindowInfo info);
    ~Window() override;

    bool shouldClose() const;
    GLFWwindow* getGLFWwindow() const { return glfwWindow; }

protected:
    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;
    virtual uint32_t acquireNextFrameIdx() override;
    virtual void submitFrame(FrameContext& frame) override;

private:
    void createWindow(const char* name, int32_t width, int32_t height);
    void createSwapchain();
    void createSyncObjects();
    void recreateSwapchain();

    SwapchainSupport querySwapchainSupport() const;
    VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupport& support) const;
    VkPresentModeKHR choosePresentMode(const SwapchainSupport& support) const;
    VkExtent2D chooseExtent(const SwapchainSupport& support) const;

    WindowInfo info;
    int32_t width, height;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR vkSurface;
    bool wasResized = false;

    struct {
        uint32_t currentInFlight = 0;
        uint32_t imageIdx = 0;
        VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
        VkSwapchainKHR vkOldSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        std::vector<VkImage> images;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
    } swapchain;
};

}


