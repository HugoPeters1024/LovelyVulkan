#include "Swapchain.h"
#include "Window.h"

namespace lv {

void Swapchain::ctor_body() {
    createSwapchain();
    transitionImages();
    createImageViews();
    createSyncObjects();
}

Swapchain::Swapchain(Window& parent, Device &device) : parent(parent), device(device) {
    ctor_body();
}

Swapchain::Swapchain(Window& parent, Device &device, std::shared_ptr<Swapchain> previous) : parent(parent), device(device), oldSwapchain(previous) {
    ctor_body();
    oldSwapchain.reset();
}

Swapchain::~Swapchain() {
    vkCheck(vkDeviceWaitIdle(device.getVkDevice()));
    for(const auto& view : vkImageViews)
        vkDestroyImageView(device.getVkDevice(), view, nullptr);
    for(const auto& sem : imageAvailableSemaphores)
        vkDestroySemaphore(device.getVkDevice(), sem, nullptr);
    for(const auto& sem : renderFinishedSemaphores)
        vkDestroySemaphore(device.getVkDevice(), sem, nullptr);
    for(const auto& fence : inFlightFences)
        vkDestroyFence(device.getVkDevice(), fence, nullptr);
    vkDestroySwapchainKHR(device.getVkDevice(), vkSwapchain, nullptr);
}

void Swapchain::createSwapchain() {
    const auto& swapchainSupport = device.getSwapchainSupportDetails(parent.getVkSurface());
    surfaceFormat = chooseSurfaceFormat(swapchainSupport);
    VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport);
    extent = chooseExtent(swapchainSupport);

    uint32_t imageCount = swapchainSupport.capabilities.maxImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);
    }

    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(device.getVkPhysicalDevice(), device.getQueueFamilyIndices().present.value(), parent.getVkSurface(), &supported);
    if (!supported) {
        logger::error("Surface of window {} is not supported for presentation", parent.getName());
        exit(1);
    }

    VkSwapchainCreateInfoKHR createInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = parent.getVkSurface(),
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapchainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->vkSwapchain,
    };

    const auto& indices = device.getQueueFamilyIndices();
    uint32_t queueFamilyIndices[2] = { indices.graphics.value(), indices.present.value() };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCheck(vkCreateSwapchainKHR(device.getVkDevice(), &createInfo, nullptr, &vkSwapchain));
    vkGetSwapchainImagesKHR(device.getVkDevice(), vkSwapchain, &imageCount, nullptr);
    vkImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getVkDevice(), vkSwapchain, &imageCount, vkImages.data());
}

void Swapchain::transitionImages() {
    for(const auto& image : vkImages) {
        device.singleTimeCommands([image](VkCommandBuffer cmdBuffer) {
            auto barrierInfo = vks::initializers::imageMemoryBarrier(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierInfo);
        });
    }
}

void Swapchain::createImageViews() {
    vkImageViews.resize(nrImages());
    for(uint32_t i=0; i<nrImages(); i++) {
        auto viewInfo = vks::initializers::imageViewCreateInfo(vkImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(device.getVkDevice(), &viewInfo, nullptr, &vkImageViews[i]));
    }
}

void Swapchain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(vkImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkCheck(vkCreateSemaphore(device.getVkDevice(), &semInfo, nullptr, &imageAvailableSemaphores[i]));
        vkCheck(vkCreateSemaphore(device.getVkDevice(), &semInfo, nullptr, &renderFinishedSemaphores[i]));
        vkCheck(vkCreateFence(device.getVkDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
    }
}

VkSurfaceFormatKHR Swapchain::chooseSurfaceFormat(const SwapchainSupportDetails &support) const {
    // Prefer SRGB
    assert(support.formats.size() > 0);
    for(const auto& format : support.formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return support.formats[0];
}

VkPresentModeKHR Swapchain::choosePresentMode(const SwapchainSupportDetails &support) const {
    for(const auto& presentMode : support.presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }

    logger::warn("Mailbox present mode not available, using FIFO ass fallback");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseExtent(const SwapchainSupportDetails &support) const {
    if (support.capabilities.currentExtent.width != 0xffffffff) {
        return support.capabilities.currentExtent;
    }

    uint32_t width = static_cast<uint32_t>(parent.getWidth());
    uint32_t height = static_cast<uint32_t>(parent.getHeight());

    return VkExtent2D {
        .width = std::clamp(width, support.capabilities.minImageExtent.width, support.capabilities.maxImageExtent.width),
        .height = std::clamp(height, support.capabilities.minImageExtent.height, support.capabilities.maxImageExtent.height),
    };
};

VkResult Swapchain::startFrame(uint32_t* imageIdx) {
    vkWaitForFences(device.getVkDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    return vkAcquireNextImageKHR(device.getVkDevice(), vkSwapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIdx);
}

VkResult Swapchain::submitFrame(const std::vector<VkCommandBuffer> cmdBuffers, uint32_t imageIdx) {
    if (imagesInFlight[imageIdx] != VK_NULL_HANDLE) {
        vkWaitForFences(device.getVkDevice(), 1, &imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIdx] = inFlightFences[currentFrame];

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailableSemaphores[currentFrame],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = static_cast<uint32_t>(cmdBuffers.size()),
        .pCommandBuffers = cmdBuffers.data(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphores[currentFrame],
        };

    vkResetFences(device.getVkDevice(), 1, &inFlightFences[currentFrame]);
    vkCheck(vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphores[currentFrame],
        .swapchainCount = 1,
        .pSwapchains = &vkSwapchain,
        .pImageIndices = &imageIdx,
        .pResults = nullptr,
    };

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);
}


}