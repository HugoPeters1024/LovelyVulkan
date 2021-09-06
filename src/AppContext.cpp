#include "AppContext.h"
#define VMA_IMPLEMENTATION
#include <vks/vk_mem_alloc.hpp>
#include "Utils.h"

namespace lv {

static struct {
    GLFWwindow* window;
    VkSurfaceKHR surface;
} windowHelper;


static bool checkValidationLayersSupported(const std::vector<const char*>& layers);
static bool deviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::set<const char*>& extensions);

AppContext::AppContext(AppContextInfo info) 
    : info(info) {
    initWindowingSystem();
    finalizeInfo();
    createInstance();
    createWindowSurface();
    pickPhysicalDevice();
    findQueueFamilies();
    createLogicalDevice();
    cleanupWindowHelper();
    createVmaAllocator();
    createCommandPool();
    createDescriptorPool();
}

AppContext::~AppContext() {
    vkDeviceWaitIdle(vkDevice);
    auto it = extensionOrder.rbegin();
    while(it != extensionOrder.rend()) {
        auto ext = extensions[*it];
        delete ext;
        it++;
    }

    vmaDestroyAllocator(vmaAllocator);
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
    vkDestroyDevice(vkDevice, nullptr);
    vkDestroyInstance(vkInstance, nullptr);
}

VkShaderModule AppContext::createShaderModule(const char* filePath) const {
    auto code = readFile(filePath);
    VkShaderModuleCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule module;
    vkCheck(vkCreateShaderModule(vkDevice, &createInfo, nullptr, &module));
    return module;
}

VkCommandBuffer AppContext::singleTimeCommandBuffer() const {
    VkCommandBuffer ret;
    auto allocInfo = vks::initializers::commandBufferAllocateInfo(vkCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    vkCheck(vkAllocateCommandBuffers(vkDevice, &allocInfo, &ret));
    auto beginInfo = vks::initializers::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(ret, &beginInfo));
    return ret;
}

void AppContext::endSingleTimeCommands(VkCommandBuffer cmdBuffer) const {
    vkCheck(vkEndCommandBuffer(cmdBuffer));
    auto submitInfo = vks::initializers::submitInfo(&cmdBuffer);
    vkQueueSubmit(queues.graphics, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queues.graphics);
}


void AppContext::initWindowingSystem() {
    // Is idempotent on multiple calls
    if (!glfwInit()) {
        logger::error("Cannot initialize GLFW");
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    windowHelper.window = glfwCreateWindow(1, 1, "Ignore", nullptr, nullptr);
    assert(windowHelper.window && "System could not create a window");
}

void AppContext::finalizeInfo() {
#ifndef NDEBUG
    info.validationLayers.insert("VK_LAYER_KHRONOS_validation");
#endif
    info.deviceExtensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.deviceExtensions.insert("VK_KHR_get_memory_requirements2");
    info.deviceExtensions.insert("VK_KHR_dedicated_allocation");
    info.deviceExtensions.insert("VK_KHR_maintenance1");

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    for(uint32_t i=0; i<glfwExtCount; i++) {
        info.instanceExtensions.insert(glfwExts[i]);
    }
}

void AppContext::createInstance() {
    std::vector<const char*> validationLayers(info.validationLayers.begin(), info.validationLayers.end());
    std::vector<const char*> instanceExtensions(info.instanceExtensions.begin(), info.instanceExtensions.end());

    if (!checkValidationLayersSupported(validationLayers)) {
        throw std::runtime_error("Not all requested validation layers supported.");
    }

    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Lovely Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "Engine Name",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data(),
    };

    vkCheck(vkCreateInstance(&createInfo, nullptr, &vkInstance));
}

void AppContext::createWindowSurface() {
    vkCheck(glfwCreateWindowSurface(vkInstance, windowHelper.window, nullptr, &windowHelper.surface));
}

void AppContext::pickPhysicalDevice() {
    uint deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("No devices with vulkan support found");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

    vkPhysicalDevice = devices[0];
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &properties);

    logger::info("Selected GPU: {}", properties.deviceName);
}


void AppContext::findQueueFamilies() {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties properties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, properties);

    for(uint i=0; i<queueFamilyCount; i++) {
        const auto& queueFamily = properties[i];
        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !queueFamilies.compute.has_value()) {
            logger::debug("Compute supported on GPU on queue {}", i);
            queueFamilies.compute = i;
        }
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !queueFamilies.graphics.has_value()) {
            logger::debug("Graphics supported on GPU on queue {}", i);
            queueFamilies.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, windowHelper.surface, &presentSupport);
        if (presentSupport && !queueFamilies.present.has_value()) {
            logger::debug("Presentation supported on GPU on queue {}", i);
            queueFamilies.present = i;
        }
    }
}

void AppContext::createLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint> uniqueQueueFamilies = {
            queueFamilies.compute.value(),
            queueFamilies.graphics.value(),
            queueFamilies.present.value(),
    };

    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    float queuePriority = 1.0f;
    for(uint queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        });
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        .samplerAnisotropy = VK_TRUE,
    };

    std::vector<const char*> devicesExtensions(info.deviceExtensions.begin(), info.deviceExtensions.end());
    if (!deviceExtensionsSupported(vkPhysicalDevice, info.deviceExtensions)) {
        logger::error("Not all device extensions supported");
        exit(1);
    }

    VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(devicesExtensions.size()),
        .ppEnabledExtensionNames = devicesExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };


    VkResult res = (vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice));

    vkGetDeviceQueue(vkDevice, queueFamilies.compute.value(), 0, &queues.compute);
    vkGetDeviceQueue(vkDevice, queueFamilies.graphics.value(), 0, &queues.graphics);
    vkGetDeviceQueue(vkDevice, queueFamilies.present.value(), 0, &queues.present);
}    

void AppContext::cleanupWindowHelper() {
    vkDestroySurfaceKHR(vkInstance, windowHelper.surface, nullptr);
    glfwDestroyWindow(windowHelper.window);
}

void AppContext::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo {
        .flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
        .physicalDevice = vkPhysicalDevice,
        .device = vkDevice,
        .instance = vkInstance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };
    vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
}

void AppContext::createCommandPool() {
    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilies.graphics.value(),
    };

    vkCheck(vkCreateCommandPool(vkDevice, &createInfo, nullptr, &vkCommandPool));
}

void AppContext::createDescriptorPool() {
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


// ---------- INTERNAL HELPER FUNCTIONS --------------


bool deviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::set<const char*>& extensions) {
    uint extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions);

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());

    for(const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


static bool checkValidationLayersSupported(const std::vector<const char*>& layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (const char* layerName : layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}





}
