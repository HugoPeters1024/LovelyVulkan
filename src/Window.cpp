#include "Window.h"

namespace lv {

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static void initSwapchain(Window& window);
static VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupport& support);
static VkPresentModeKHR choosePresentMode(const SwapchainSupport& support);
static VkExtent2D chooseExtent(const Window& window, const SwapchainSupport& support);

static SwapchainSupport querySwapchainSupport(Window& window);

void createWindow(AppContext& ctx, const char* name, int32_t width, int32_t height, Window& dst) {
    dst.ctx = &ctx;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    dst.glfwWindow = glfwCreateWindow(width, height, name, nullptr, nullptr);
    vkCheck(glfwCreateWindowSurface(ctx.vkInstance, dst.glfwWindow, nullptr, &dst.vkSurface));
    initSwapchain(dst);
}

void destroyWindow(Window& window) {
    auto& device = window.ctx->vkDevice;
    vkDeviceWaitIdle(device);
    for(auto& view : window.swapchain.imageViews)
        vkDestroyImageView(device, view, nullptr);

    for(auto& sem : window.swapchain.renderFinishedSemaphores)
        vkDestroySemaphore(device, sem, nullptr);
    
    for(auto& sem : window.swapchain.imageAvailableSemaphores)
        vkDestroySemaphore(device, sem, nullptr);

    for(auto& fence : window.swapchain.inFlightFences)
        vkDestroyFence(device, fence, nullptr);

    vkDestroySwapchainKHR(device, window.swapchain.vkSwapchain, nullptr);
    vkDestroySurfaceKHR(window.ctx->vkInstance, window.vkSurface, nullptr);
}


bool windowShouldClose(const Window& window) {
    return glfwWindowShouldClose(window.glfwWindow);
}

const FrameContext* nextFrame(Window& window) {
    uint32_t& currentFrame = window.swapchain.currentFrame;
    glfwPollEvents();

    vkWaitForFences(window.ctx->vkDevice, 1, &window.swapchain.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    auto result = vkAcquireNextImageKHR(window.ctx->vkDevice, window.swapchain.vkSwapchain, UINT64_MAX, window.swapchain.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &window.swapchain.imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        initSwapchain(window);
        return nextFrame(window);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        logger::error("Failed to acquire swapchain image!");
        exit(1);
    }

    auto& frame = window.swapchain.frameContexts[currentFrame];
    vkCheck(vkResetCommandBuffer(frame.commandBuffer, 0));
    auto beginInfo = vks::initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo));
    return &frame;
}

void commitFrame(Window& window) {
    auto& currentFrame = window.swapchain.currentFrame;
    auto& frame = window.swapchain.frameContexts[currentFrame];
    vkEndCommandBuffer(frame.commandBuffer);

    if (window.swapchain.imagesInFlight[window.swapchain.imageIdx]) {
        vkCheck(vkWaitForFences(window.ctx->vkDevice, 1, &window.swapchain.imagesInFlight[window.swapchain.imageIdx], VK_TRUE, UINT64_MAX));
    }
;
    window.swapchain.imagesInFlight[window.swapchain.imageIdx] = window.swapchain.inFlightFences[currentFrame];
    auto submitInfo = vks::initializers::submitInfo(&window.swapchain.frameContexts[currentFrame].commandBuffer);
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &window.swapchain.imageAvailableSemaphores[currentFrame];
    VkShaderStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &window.swapchain.renderFinishedSemaphores[currentFrame]; 

    vkCheck(vkResetFences(window.ctx->vkDevice, 1, &window.swapchain.inFlightFences[currentFrame]));
    vkCheck(vkQueueSubmit(window.ctx->queues.graphics, 1, &submitInfo, window.swapchain.inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &window.swapchain.renderFinishedSemaphores[currentFrame],
            .swapchainCount = 1,
            .pSwapchains = &window.swapchain.vkSwapchain,
            .pImageIndices = &window.swapchain.imageIdx,
            .pResults = nullptr,
    };

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    vkCheck(vkQueuePresentKHR(window.ctx->queues.present, &presentInfo));
}

void getFramebufferSize(const Window& window, int32_t* width, int32_t* height) {
    glfwGetFramebufferSize(window.glfwWindow, width, height);
}

static void initSwapchain(Window& window) {

    const auto& support = querySwapchainSupport(window);
    window.swapchain.surfaceFormat = chooseSurfaceFormat(support);
    auto presentMode = choosePresentMode(support);
    window.swapchain.extent = chooseExtent(window, support);

    const auto& capabilities = support.capabilities;
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = window.vkSurface,
        .minImageCount = imageCount,
        .imageFormat = window.swapchain.surfaceFormat.format,
        .imageColorSpace = window.swapchain.surfaceFormat.colorSpace,
        .imageExtent = window.swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = window.swapchain.vkSwapchain,
    };

    uint32_t queueFamilyIndices[2] = { window.ctx->queueFamilies.graphics.value(), window.ctx->queueFamilies.present.value() }; 

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCheck(vkCreateSwapchainKHR(window.ctx->vkDevice, &createInfo, nullptr, &window.swapchain.vkSwapchain));
    vkGetSwapchainImagesKHR(window.ctx->vkDevice, window.swapchain.vkSwapchain, &imageCount, nullptr);
    window.swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(window.ctx->vkDevice, window.swapchain.vkSwapchain, &imageCount, window.swapchain.images.data());


    // Create image views
    window.swapchain.imageViews.resize(imageCount);
    for(uint32_t i=0; i<imageCount; i++) {
        auto viewInfo = vks::initializers::imageViewCreateInfo(window.swapchain.images[i], window.swapchain.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(window.ctx->vkDevice, &viewInfo, nullptr, &window.swapchain.imageViews[i]));
    }

    // Create sync objects
    window.swapchain.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    window.swapchain.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    window.swapchain.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    window.swapchain.imagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    auto semInfo = vks::initializers::semaphoreCreateInfo();
    auto fenceInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for(uint32_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkCheck(vkCreateSemaphore(window.ctx->vkDevice, &semInfo, nullptr, &window.swapchain.imageAvailableSemaphores[i]));
        vkCheck(vkCreateSemaphore(window.ctx->vkDevice, &semInfo, nullptr, &window.swapchain.renderFinishedSemaphores[i]));
        vkCheck(vkCreateFence(window.ctx->vkDevice, &fenceInfo, nullptr, &window.swapchain.inFlightFences[i]));
    }

    // create the command buffers
    window.swapchain.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    auto allocInfo = vks::initializers::commandBufferAllocateInfo(window.ctx->vkCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
    vkCheck(vkAllocateCommandBuffers(window.ctx->vkDevice, &allocInfo, window.swapchain.commandBuffers.data()));

    // create the frameContexts
    window.swapchain.frameContexts.resize(MAX_FRAMES_IN_FLIGHT);
    for(uint32_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        window.swapchain.frameContexts[i].commandBuffer = window.swapchain.commandBuffers[i];
    }
}

static SwapchainSupport querySwapchainSupport(Window& window) {
    assert(window.ctx);
    // validation layers require this query to be done
    VkBool32 presentSupport = VK_FALSE;
    vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(window.ctx->vkPhysicalDevice, window.ctx->queueFamilies.present.value(), window.vkSurface, &presentSupport));
    assert(presentSupport);

    SwapchainSupport details{};
    vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(window.ctx->vkPhysicalDevice, window.vkSurface, &details.capabilities));

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(window.ctx->vkPhysicalDevice, window.vkSurface, &formatCount, nullptr);

    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(window.ctx->vkPhysicalDevice, window.vkSurface, &formatCount, details.formats.data());
    }

    uint presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(window.ctx->vkPhysicalDevice, window.vkSurface, &presentCount, nullptr);

    if (presentCount > 0) {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(window.ctx->vkPhysicalDevice, window.vkSurface, &presentCount, details.presentModes.data());
    }

    return details;
}

static VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupport& support) {
    // Prefer SRGB
    const auto& availableFormats = support.formats;
    for(const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

static VkPresentModeKHR choosePresentMode(const SwapchainSupport& support) {
    const auto& availablePresentModes = support.presentModes;
    for(const auto& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // Always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseExtent(const Window& window, const SwapchainSupport& support) {
    const auto& capabilities = support.capabilities;

    if (capabilities.currentExtent.width != 0xffffffff) {
        return capabilities.currentExtent;
    }

    int width, height;
    getFramebufferSize(window, &width, &height);

    return VkExtent2D {
        .width = std::clamp(static_cast<uint>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<uint>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

}
