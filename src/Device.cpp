#include "Device.h"
#define VMA_IMPLEMENTATION
#include <vks/vk_mem_alloc.hpp>

namespace lv {

Device::Device(lv::DeviceInfo info) : info(std::move(info)) {
#ifndef NDEBUG
    logger::set_level(logger::level::debug);
#endif
    if (!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }
    finalizeInfo();
    ensureValidationLayersSupported();
    createInstance();
    auto dummyWindow = std::make_unique<DummyWindow>(vkInstance);
    pickPhysicalDevice(dummyWindow->getSurface());
    createLogicalDevice();
    logger::info("Device setup and ready for use");
}

Device::~Device() {
    logger::info("Device being destroyed");
    windows.clear();
    glfwTerminate();
    vkDestroyDevice(vkDevice, nullptr);
    vkDestroyInstance(vkInstance, nullptr);
}

void Device::finalizeInfo() {
    DummyWindow::collectInstanceExtensions(info.instanceExtensions);
}

void Device::ensureValidationLayersSupported() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const auto& layerName : info.validationLayers) {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers) {
            if (strcmp(layerName.c_str(), layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            logger::error("Requested validation layer {} not supported", layerName);
            exit(1);
        }
    }
}

void Device::createInstance() {

    std::vector<const char*> validationLayers;
    std::transform(info.validationLayers.begin(), info.validationLayers.end(),
                   std::back_inserter(validationLayers),
                   [](const std::string& el) { return el.c_str(); });

    std::vector<const char*> instanceExtensions;
    std::transform(info.instanceExtensions.begin(), info.instanceExtensions.end(),
                   std::back_inserter(instanceExtensions),
                   [](const std::string& el) { return el.c_str(); });

    auto appInfo = vks::initializers::applicationInfo(info.apiVersion);
    auto instanceInfo = vks::initializers::instanceInfo(&appInfo, validationLayers, instanceExtensions);
    vkCheck(vkCreateInstance(&instanceInfo, nullptr, &vkInstance));
}

void Device::pickPhysicalDevice(VkSurfaceKHR dummySurface) {
    uint32_t deviceCount = 1;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        logger::error("No devices with vulkan support found!");
        exit(1);
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

    // TODO: implement selection logic
    vkPhysicalDevice = devices[0];
    queueFamilyIndices = findQueueFamilyIndices(vkPhysicalDevice, dummySurface);
    assert(queueFamilyIndices.isComplete());

    logger::info("Running on GPU: {}", getDeviceName());
}

void Device::createLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = queueFamilyIndices.uniqueIndices();
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.push_back(vks::initializers::queueCreateInfo(queueFamily, &queuePriority));
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    std::vector<const char*> deviceExtensions;
    std::transform(info.deviceExtensions.begin(), info.deviceExtensions.end(),
                   std::back_inserter(deviceExtensions),
                   [](const std::string& el) { return el.c_str(); });

    auto deviceInfo = vks::initializers::deviceCreateInfo(queueCreateInfos, deviceExtensions, &deviceFeatures);
    vkCheck(vkCreateDevice(vkPhysicalDevice, &deviceInfo, nullptr, &vkDevice));

    vkGetDeviceQueue(vkDevice, queueFamilyIndices.compute.value(), 0, &computeQueue);
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphics.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.present.value(), 0, &presentQueue);
}

void Device::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
        .physicalDevice = vkPhysicalDevice,
        .device = vkDevice,
        .instance = vkInstance,
        .vulkanApiVersion = info.apiVersion,
    };

    vkCheck(vmaCreateAllocator(&allocatorInfo, &vmaAllocator));
}

QueueFamilyIndices Device::findQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices ret{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties properties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, properties);

    for(uint32_t i=0; i<queueFamilyCount; i++) {
        const auto& queueFamily = properties[i];
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) ret.compute = i;
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) ret.graphics = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) ret.present = i;
    }

    return ret;
}

Window *Device::createWindow(const std::string &name, int width, int height) {
    windows.push_back(std::make_unique<Window>(*this, name, width, height));
    return windows.back().get();
}

std::string Device::getDeviceName() const {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
    return deviceProperties.deviceName;
}

}


