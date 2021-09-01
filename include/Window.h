#pragma once
#include "precomp.h"


#define MAX_FRAMES_IN_FLIGHT 3

namespace lv {

class AppContext;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct FrameContext {
    struct {
        VkImage vkImage;
        VkImageView vkView;
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;
    } swapchain;
    VkCommandBuffer commandBuffer;
};

class Window : NoCopy {
public:
    AppContext& ctx;
    int32_t width, height;
    std::string name;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR vkSurface;
    bool wasResized = false;

    struct {
        uint32_t currentFrame = 0;
        uint32_t imageIdx = 0;
        VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
        VkSwapchainKHR vkOldSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        std::vector<FrameContext> frameContexts;
        std::vector<VkFence> imagesInFlight;
    } swapchain;

public:
    Window(AppContext& ctx, const char* name, int32_t width, int32_t height);
    ~Window();

    bool shouldClose() const;
    void nextFrame(std::function<void(FrameContext&)> callback);


private:
    void createWindow(const char* name, int32_t width, int32_t height);
    void createSwapchain();
    void recreateSwapchain();

    SwapchainSupport querySwapchainSupport() const;
    VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupport& support) const;
    VkPresentModeKHR choosePresentMode(const SwapchainSupport& support) const;
    VkExtent2D chooseExtent(const SwapchainSupport& support) const;
};

}

