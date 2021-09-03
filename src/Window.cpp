#include "Window.h"
#include "AppContext.h"

namespace lv {


Window::Window(AppContext& ctx, const char* name, int32_t width, int32_t height) 
    : ctx(ctx) {
    createWindow(name, width, height);
    createSwapchain();
    buildExtensionFrames();
}

Window::~Window() {
    vkDeviceWaitIdle(ctx.vkDevice);
    for(auto& frame : swapchain.frameContexts) {
        vkDestroyImageView(ctx.vkDevice, frame.swapchain.vkView, nullptr);
        vkDestroySemaphore(ctx.vkDevice, frame.swapchain.imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(ctx.vkDevice, frame.swapchain.renderFinishedSemaphore, nullptr);
        vkDestroyFence(ctx.vkDevice, frame.swapchain.inFlightFence, nullptr);
    }

    vkDestroySwapchainKHR(ctx.vkDevice, swapchain.vkSwapchain, nullptr);
    vkDestroySurfaceKHR(ctx.vkInstance, vkSurface, nullptr);
    glfwDestroyWindow(glfwWindow);
}

void Window::createWindow(const char* name, int32_t width, int32_t height) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(width, height, name, nullptr, nullptr);
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

    vkGetSwapchainImagesKHR(ctx.vkDevice, swapchain.vkSwapchain, &imageCount, nullptr);

    VkImage images[imageCount];
    vkGetSwapchainImagesKHR(ctx.vkDevice, swapchain.vkSwapchain, &imageCount, images);
    for(uint32_t i=0; i<imageCount; i++) {
        swapchain.frameContexts.push_back(FrameContext(ctx));
        swapchain.frameContexts[i].swapchain.vkImage = images[i];
    }

    // Create image views
    for(uint32_t i=0; i<imageCount; i++) {
        auto& frame = swapchain.frameContexts[i];
        auto viewInfo = vks::initializers::imageViewCreateInfo(frame.swapchain.vkImage, swapchain.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

        vkCheck(vkCreateImageView(ctx.vkDevice, &viewInfo, nullptr, &frame.swapchain.vkView));
    }

    // Create sync objects
    auto semInfo = vks::initializers::semaphoreCreateInfo();
    auto fenceInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    for(uint32_t i=0; i<imageCount; i++) {
        auto& frame = swapchain.frameContexts[i];
        vkCheck(vkCreateSemaphore(ctx.vkDevice, &semInfo, nullptr, &frame.swapchain.imageAvailableSemaphore));
        vkCheck(vkCreateSemaphore(ctx.vkDevice, &semInfo, nullptr, &frame.swapchain.renderFinishedSemaphore));
        vkCheck(vkCreateFence(ctx.vkDevice, &fenceInfo, nullptr, &frame.swapchain.inFlightFence));
    }
    swapchain.imagesInFlight.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

    // create the command buffers
    for(uint32_t i=0; i<imageCount; i++) {
        auto& frame = swapchain.frameContexts[i];
        auto allocInfo = vks::initializers::commandBufferAllocateInfo(ctx.vkCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
        vkCheck(vkAllocateCommandBuffers(ctx.vkDevice, &allocInfo, &frame.commandBuffer));
    }
}

void Window::buildExtensionFrames() {
    for(auto& pair : ctx.extensions) {
        for(auto& frame : swapchain.frameContexts) {
            frame.extensionFrame.insert({pair.second->frameType(), pair.second->buildDowncastedFrame(frame)});
        }
    }
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

void Window::nextFrame(std::function<void(FrameContext&)> callback) {
    FrameContext& frame = swapchain.frameContexts[swapchain.currentFrame];
    glfwPollEvents();

    vkWaitForFences(ctx.vkDevice, 1, &frame.swapchain.inFlightFence, VK_TRUE, UINT64_MAX);

    auto result = vkAcquireNextImageKHR(ctx.vkDevice, swapchain.vkSwapchain, UINT64_MAX, frame.swapchain.imageAvailableSemaphore, VK_NULL_HANDLE, &swapchain.imageIdx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return nextFrame(callback);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        logger::error("Failed to acquire swapchain image!");
        exit(1);
    }

    vkCheck(vkResetCommandBuffer(frame.commandBuffer, 0));
    auto beginInfo = vks::initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo));

    callback(frame);

    vkEndCommandBuffer(frame.commandBuffer);

    if (swapchain.imagesInFlight[swapchain.imageIdx] != VK_NULL_HANDLE) {
        vkWaitForFences(ctx.vkDevice, 1, &swapchain.imagesInFlight[swapchain.currentFrame], VK_TRUE, UINT64_MAX);
    }
    swapchain.imagesInFlight[swapchain.imageIdx] = frame.swapchain.inFlightFence;

    auto submitInfo = vks::initializers::submitInfo(&frame.commandBuffer);
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.swapchain.imageAvailableSemaphore;
    VkShaderStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame.swapchain.renderFinishedSemaphore; 

    vkCheck(vkResetFences(ctx.vkDevice, 1, &frame.swapchain.inFlightFence));
    vkCheck(vkQueueSubmit(ctx.queues.graphics, 1, &submitInfo, frame.swapchain.inFlightFence));

    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame.swapchain.renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.vkSwapchain,
            .pImageIndices = &swapchain.imageIdx,
            .pResults = nullptr,
    };

    swapchain.currentFrame = (swapchain.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    vkCheck(vkQueuePresentKHR(ctx.queues.present, &presentInfo));
}


}
