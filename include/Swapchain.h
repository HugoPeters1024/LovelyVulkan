#pragma once
#include "precomp.h"
#include "Device.h"

namespace lv {

class Window;

class Swapchain : NoCopy {
private:
    Device& device;
    Window& parent;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;
    VkSwapchainKHR vkSwapchain;
    std::shared_ptr<Swapchain> oldSwapchain;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    std::vector<VkImage> vkImages;
    std::vector<VkImageView> vkImageViews;
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;


    void ctor_body();
    void createSwapchain();
    void transitionImages();
    void createImageViews();
    void createSyncObjects();

    VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupportDetails& support) const;
    VkPresentModeKHR choosePresentMode(const SwapchainSupportDetails& support) const;
    VkExtent2D chooseExtent(const SwapchainSupportDetails& support) const;

public:
    Swapchain(Window& parent, Device& device);
    Swapchain(Window& parent, Device& device, std::shared_ptr<Swapchain> previous);
    ~Swapchain();

    uint32_t nrImages() const { return static_cast<uint32_t>(vkImages.size()); }
    const VkSurfaceFormatKHR &getSurfaceFormat() const { return surfaceFormat; }
    const VkExtent2D &getExtent() const { return extent; }

    VkResult startFrame(uint32_t* imageIdx);
    VkResult submitFrame(const std::vector<VkCommandBuffer> cmdBuffers, uint32_t imageIdx);
};

}