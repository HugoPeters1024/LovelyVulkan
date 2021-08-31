#pragma once
#include "precomp.h"

namespace lv {

struct AppContextInfo {
    uint32_t apiVersion = VK_API_VERSION_1_2;
    std::set<const char*> validationLayers;
    std::set<const char*> instanceExtensions;
    std::set<const char*> deviceExtensions;
};

struct AppContext : NoCopy {
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice vkDevice;
    VmaAllocator vmaAllocator;

    struct {
        std::optional<uint32_t> compute, graphics, present;
    } queueFamilies;

    struct {
        VkQueue compute, graphics, present;
    } queues;

    VkCommandPool vkCommandPool;
    VkDescriptorPool vkDescriptorPool;

    struct {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    } swapchainSupport;
};

struct FrameContext {
    VkImageView swapchainImage;
    VkCommandBuffer commandBuffer;
};

struct Window : NoCopy {
    AppContext* ctx;
    int32_t width, height;
    std::string name;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR vkSurface;
    bool wasResized = false;

    struct {
        uint32_t currentFrame = 0;
        VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<FrameContext> frameContexts;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
    } swapchain;
};


}
