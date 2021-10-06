#include "Window.h"
#include "AppContext.h"

namespace lv {

Window::Window(AppContext& ctx, WindowInfo info)
    : FrameManager(ctx), info(info) {
    createWindow(info.windowName.c_str(), info.width, info.height);
    createSwapchain();
    createSyncObjects();
}

Window::~Window() {
    for(auto& frame : frameContexts) {
        auto& wFrame = frame.getExtFrame<WindowFrame>();
        vkDestroyImageView(ctx.vkDevice, wFrame.vkView, nullptr);
    }
    vkDestroySwapchainKHR(ctx.vkDevice, swapchain.vkSwapchain, nullptr);
    vkDestroySurfaceKHR(ctx.vkInstance, vkSurface, nullptr);
    for(const auto& sem : swapchain.imageAvailableSemaphores) {
        vkDestroySemaphore(ctx.vkDevice, sem, nullptr);
    }
    for(const auto& sem : swapchain.renderFinishedSemaphores) {
        vkDestroySemaphore(ctx.vkDevice, sem, nullptr);
    }
    glfwDestroyWindow(glfwWindow);
}

void Window::createWindow(const char* name, int32_t width, int32_t height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(info.width, info.height, name, nullptr, nullptr);
    vkCheck(glfwCreateWindowSurface(ctx.vkInstance, glfwWindow, nullptr, &vkSurface));
}

void Window::createSwapchain() {

    const auto& support = querySwapchainSupport();
    swapchain.surfaceFormat = chooseSurfaceFormat(support);
    auto presentMode = choosePresentMode(support);
    swapchain.extent = chooseExtent(support);

    const auto& capabilities = support.capabilities;
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkSurface,
        .minImageCount = imageCount,
        .imageFormat = swapchain.surfaceFormat.format,
        .imageColorSpace = swapchain.surfaceFormat.colorSpace,
        .imageExtent = swapchain.extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain.vkSwapchain,
    };

    uint32_t queueFamilyIndices[2] = { ctx.queueFamilies.graphics.value(), ctx.queueFamilies.present.value() };

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCheck(vkCreateSwapchainKHR(ctx.vkDevice, &createInfo, nullptr, &swapchain.vkSwapchain));

    // Retrieve the images
    vkGetSwapchainImagesKHR(ctx.vkDevice, swapchain.vkSwapchain, &imageCount, nullptr);
    swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.vkDevice, swapchain.vkSwapchain, &imageCount, swapchain.images.data());

    setNrFrames(imageCount);
}

void Window::createSyncObjects() {
    swapchain.imageAvailableSemaphores.resize(nrFramesInFlight);
    swapchain.renderFinishedSemaphores.resize(nrFramesInFlight);

    // Create the semaphores
    for(uint32_t i=0; i<nrFramesInFlight; i++) {
        auto semInfo = vks::initializers::semaphoreCreateInfo();
        vkCheck(vkCreateSemaphore(ctx.vkDevice, &semInfo, nullptr, &swapchain.imageAvailableSemaphores[i]));
        vkCheck(vkCreateSemaphore(ctx.vkDevice, &semInfo, nullptr, &swapchain.renderFinishedSemaphores[i]));
    }
}

void Window::embellishFrameContext(FrameContext& frame) {
    auto& windowFrame = frame.registerExtFrame<WindowFrame>();

    windowFrame.width = info.width;
    windowFrame.height = info.height;
    windowFrame.format = swapchain.surfaceFormat.format;
    windowFrame.vkImage = swapchain.images[frame.idx];

    // Create image views
    auto viewInfo = vks::initializers::imageViewCreateInfo(windowFrame.vkImage, swapchain.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCheck(vkCreateImageView(ctx.vkDevice, &viewInfo, nullptr, &windowFrame.vkView));

}

void Window::cleanupFrameContext(FrameContext &frame) {
    auto& windowFrame = frame.registerExtFrame<WindowFrame>();
    vkDestroyImageView(ctx.vkDevice, windowFrame.vkView, nullptr);
}

void Window::recreateSwapchain() {
    throw std::runtime_error("TODO");
}

SwapchainSupport Window::querySwapchainSupport() const {
    // validation layers require this query to be done
    VkBool32 presentSupport = VK_FALSE;
    vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(ctx.vkPhysicalDevice, ctx.queueFamilies.present.value(), vkSurface, &presentSupport));
    assert(presentSupport);

    SwapchainSupport details{};
    vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.vkPhysicalDevice, vkSurface, &details.capabilities));

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.vkPhysicalDevice, vkSurface, &formatCount, nullptr);

    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.vkPhysicalDevice, vkSurface, &formatCount, details.formats.data());
    }

    uint presentCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vkPhysicalDevice, vkSurface, &presentCount, nullptr);

    if (presentCount > 0) {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.vkPhysicalDevice, vkSurface, &presentCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Window::chooseSurfaceFormat(const SwapchainSupport& support) const {
    // Prefer SRGB
    const auto& availableFormats = support.formats;
    for(const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Window::choosePresentMode(const SwapchainSupport& support) const {
    const auto& availablePresentModes = support.presentModes;
    for(const auto& presentMode : availablePresentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    // Always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Window::chooseExtent(const SwapchainSupport& support) const {
    const auto& capabilities = support.capabilities;

    if (capabilities.currentExtent.width != 0xffffffff) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);

    return VkExtent2D {
        .width = std::clamp(static_cast<uint>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<uint>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

//------------- PUBLIC FUNCTIONS -------------------

bool Window::shouldClose() const {
    return glfwWindowShouldClose(glfwWindow);
}

uint32_t Window::acquireNextFrameIdx() {
    glfwPollEvents();

    auto result = vkAcquireNextImageKHR(ctx.vkDevice, swapchain.vkSwapchain, UINT64_MAX, swapchain.imageAvailableSemaphores[currentInFlight], VK_NULL_HANDLE, &swapchain.imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return acquireNextFrameIdx();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        logger::error("Failed to acquire swapchain image!");
        exit(1);
    }

    return swapchain.imageIdx;
}

void Window::submitFrame(FrameContext& frame) {
    auto& wFrame = frame.getExtFrame<WindowFrame>();
    auto submitInfo = vks::initializers::submitInfo(&frame.cmdBuffer);
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &swapchain.imageAvailableSemaphores[currentInFlight];
    VkShaderStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &swapchain.renderFinishedSemaphores[currentInFlight];

    vkCheck(vkQueueSubmit(ctx.queues.graphics, 1, &submitInfo, frame.frameFinished));

    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &swapchain.renderFinishedSemaphores[currentInFlight],
            .swapchainCount = 1,
            .pSwapchains = &swapchain.vkSwapchain,
            .pImageIndices = &swapchain.imageIdx,
            .pResults = nullptr,
    };

    vkCheck(vkQueuePresentKHR(ctx.queues.present, &presentInfo));
}


}
