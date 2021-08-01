#pragma once
#include "precomp.h"
#include "DummyWindow.h"

namespace lv {

class Window;

struct DeviceInfo {
    uint32_t apiVersion = VK_API_VERSION_1_2;
    std::set<std::string> validationLayers;
    std::set<std::string> instanceExtensions;
    std::set<std::string> deviceExtensions;

    DeviceInfo() {
#ifndef NDEBUG
        validationLayers.insert("VK_LAYER_KHRONOS_validation");
#endif
        deviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceExtensions.insert("VK_KHR_get_memory_requirements2");
        deviceExtensions.insert("VK_KHR_dedicated_allocation");
        deviceExtensions.insert("VK_KHR_maintenance1");
    }
};

struct QueueFamilyIndices {
    std::optional<uint32_t> compute;
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() const { return compute.has_value() && graphics.has_value() && present.has_value(); }
    std::set<uint32_t> uniqueIndices() const { assert(isComplete()); return std::set<uint32_t> { compute.value(), graphics.value(), present.value() };}
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device {
private:
    void finalizeInfo();
    void ensureValidationLayersSupported();
    void createInstance();
    void pickPhysicalDevice(VkSurfaceKHR dummySurface);
    void createLogicalDevice();
    void createVmaAllocator();
    void createCommandPool();

    static QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);


    DeviceInfo info;
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice vkDevice;
    VkQueue computeQueue, graphicsQueue, presentQueue;
    VmaAllocator vmaAllocator;
    QueueFamilyIndices queueFamilyIndices;
    VkCommandPool vkCommandPool;

    std::vector<std::unique_ptr<Window>> windows;

public:
    Device(DeviceInfo info);
    ~Device();

    Window* createWindow(const std::string& name, int width, int height);
    std::string getDeviceName() const;
    SwapchainSupportDetails getSwapchainSupportDetails(VkSurfaceKHR surface) const;
    VkCommandBuffer beginSingleTimeCommands();
    void submitSingleTimeCommands(VkCommandBuffer cmdBuffer);
    void singleTimeCommands(const std::function<void(VkCommandBuffer)>& callback);

    const VkInstance getVkInstance() const { return vkInstance; }
    const VkPhysicalDevice getVkPhysicalDevice() const { return vkPhysicalDevice; }
    const VkDevice getVkDevice() const { return vkDevice; }
    const QueueFamilyIndices &getQueueFamilyIndices() const { return queueFamilyIndices; }
    VkQueue getComputeQueue() const { return computeQueue; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
};

}
