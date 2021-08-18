#include "Device.h"
#include "Window.h"
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
    createVmaAllocator();
    createCommandPool();
    createDescriptorPool();
    logger::info("Device setup and ready for use");
}

Device::~Device() {
    logger::info("Device being destroyed");
    windows.clear();
    glfwTerminate();
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
    vmaDestroyAllocator(vmaAllocator);
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

void Device::createCommandPool() {
    auto createInfo = vks::initializers::commandPoolCreateInfo(queueFamilyIndices.graphics.value());
    vkCheck(vkCreateCommandPool(vkDevice, &createInfo, nullptr, &vkCommandPool));
}

void Device::createDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] =
            {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

    VkDescriptorPoolCreateInfo poolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 100,
        .poolSizeCount = std::size(pool_sizes),
        .pPoolSizes = pool_sizes,
        };

    vkCheck(vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool));
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

VkCommandBuffer Device::beginSingleTimeCommands() {
    auto allocInfo = vks::initializers::commandBufferAllocateInfo(vkCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer ret;
    vkCheck(vkAllocateCommandBuffers(vkDevice, &allocInfo, &ret));

    auto beginInfo = vks::initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkCheck(vkBeginCommandBuffer(ret, &beginInfo));
    return ret;
}

void Device::submitSingleTimeCommands(VkCommandBuffer cmdBuffer) {
    vkCheck(vkEndCommandBuffer(cmdBuffer));
    auto submitInfo = vks::initializers::submitInfo(&cmdBuffer);
    vkCheck(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &cmdBuffer);
}

void Device::singleTimeCommands(const std::function<void(VkCommandBuffer)>& callback) {
    auto cmdBuffer = beginSingleTimeCommands();
    callback(cmdBuffer);
    submitSingleTimeCommands(cmdBuffer);
}

void Device::createDeviceImage(VkImageCreateInfo imageInfo, VkImage *image, VmaAllocation *memory) {
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY, };
    vkCheck(vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, image, memory, nullptr));
}

VkShaderModule Device::createShaderModule(const std::string &filename) const {
    auto code = readFile(filename);
    logger::debug("Shader module {} contains {} bytes", filename, code.size());
    VkShaderModuleCreateInfo moduleInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule module;
    vkCheck(vkCreateShaderModule(vkDevice, &moduleInfo, nullptr, &module));
    return module;
}

std::string Device::getDeviceName() const {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
    return deviceProperties.deviceName;
}

SwapchainSupportDetails Device::getSwapchainSupportDetails(VkSurfaceKHR surface) const {
    SwapchainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount, nullptr);
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &presentCount, nullptr);
    if (presentCount > 0) {
        details.presentModes.resize(presentCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &presentCount, details.presentModes.data());
    }

    return details;
}

}


