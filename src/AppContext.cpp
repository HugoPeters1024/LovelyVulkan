#include "AppContext.h"
#define VMA_IMPLEMENTATION
#include <vks/vk_mem_alloc.hpp>

namespace lv {

static struct {
    GLFWwindow* window;
    VkSurfaceKHR surface;
} windowHelper;


static void initWindowingSystem();
static void finalizeInfo(AppContextInfo& info);
static void createInstance(AppContextInfo& info, AppContext& ctx); 
static void createWindowSurface(AppContext& ctx);
static void pickPhysicalDevice(AppContextInfo& info, AppContext& ctx);
static void findQueueFamilies(AppContext& ctx);
static void createLogicalDevice(AppContextInfo& info, AppContext& ctx);
static void cleanWindowHelper(AppContext& ctx);
static void createCommandPool(AppContext& ctx);
static void createDescriptorPool(AppContext& ctx);

static bool checkValidationLayersSupported(const std::vector<const char*>& layers);
static bool deviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::set<const char*>& extensions);

void createAppContext(AppContextInfo& info, AppContext& ctx) {
    initWindowingSystem();
    finalizeInfo(info);
    createInstance(info, ctx);
    createWindowSurface(ctx);
    pickPhysicalDevice(info, ctx);
    findQueueFamilies(ctx);
    createLogicalDevice(info, ctx);
    cleanWindowHelper(ctx);
    createCommandPool(ctx);
    createDescriptorPool(ctx);
}

void destroyAppContext(AppContext& ctx) {
    vkDeviceWaitIdle(ctx.vkDevice);
    vkDestroyDescriptorPool(ctx.vkDevice, ctx.vkDescriptorPool, nullptr);
    vkDestroyCommandPool(ctx.vkDevice, ctx.vkCommandPool, nullptr);
    vkDestroyDevice(ctx.vkDevice, nullptr);
    vkDestroyInstance(ctx.vkInstance, nullptr);
}

// ---------- INTERNAL HELPER FUNCTIONS --------------

static void initWindowingSystem() {
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


static void finalizeInfo(AppContextInfo& info) {
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

static void createInstance(AppContextInfo& info, AppContext& ctx) {
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

    vkCheck(vkCreateInstance(&createInfo, nullptr, &ctx.vkInstance));
}

static void createWindowSurface(AppContext& ctx) {
    vkCheck(glfwCreateWindowSurface(ctx.vkInstance, windowHelper.window, nullptr, &windowHelper.surface));
}

static void pickPhysicalDevice(AppContextInfo& info, AppContext& ctx) {
    uint deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.vkInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("No devices with vulkan support found");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(ctx.vkInstance, &deviceCount, devices);

    ctx.vkPhysicalDevice = devices[0];
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(ctx.vkPhysicalDevice, &properties);

    logger::info("Selected GPU: {}", properties.deviceName);
}

static bool deviceExtensionsSupported(VkPhysicalDevice physicalDevice, const std::set<const char*>& extensions) {
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

static void findQueueFamilies(AppContext& ctx) {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.vkPhysicalDevice, &queueFamilyCount, nullptr);
    VkQueueFamilyProperties properties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.vkPhysicalDevice, &queueFamilyCount, properties);

    for(uint i=0; i<queueFamilyCount; i++) {
        const auto& queueFamily = properties[i];
        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !ctx.queueFamilies.compute.has_value()) {
            logger::debug("Compute supported on GPU on queue {}", i);
            ctx.queueFamilies.compute = i;
        }
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !ctx.queueFamilies.graphics.has_value()) {
            logger::debug("Graphics supported on GPU on queue {}", i);
            ctx.queueFamilies.graphics = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.vkPhysicalDevice, i, windowHelper.surface, &presentSupport);
        if (presentSupport && !ctx.queueFamilies.present.has_value()) {
            logger::debug("Presentation supported on GPU on queue {}", i);
            ctx.queueFamilies.present = i;
        }
    }
}

static void createLogicalDevice(AppContextInfo& info, AppContext& ctx) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint> uniqueQueueFamilies = {
            ctx.queueFamilies.compute.value(),
            ctx.queueFamilies.graphics.value(),
            ctx.queueFamilies.present.value(),
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
    if (!deviceExtensionsSupported(ctx.vkPhysicalDevice, info.deviceExtensions)) {
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


    VkResult res = (vkCreateDevice(ctx.vkPhysicalDevice, &createInfo, nullptr, &ctx.vkDevice));

    vkGetDeviceQueue(ctx.vkDevice, ctx.queueFamilies.compute.value(), 0, &ctx.queues.compute);
    vkGetDeviceQueue(ctx.vkDevice, ctx.queueFamilies.graphics.value(), 0, &ctx.queues.graphics);
    vkGetDeviceQueue(ctx.vkDevice, ctx.queueFamilies.present.value(), 0, &ctx.queues.present);
}    

static void cleanWindowHelper(AppContext& ctx) {
    vkDestroySurfaceKHR(ctx.vkInstance, windowHelper.surface, nullptr);
    glfwDestroyWindow(windowHelper.window);
}

static void createCommandPool(AppContext& ctx) {
    VkCommandPoolCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.queueFamilies.graphics.value(),
    };

    vkCheck(vkCreateCommandPool(ctx.vkDevice, &createInfo, nullptr, &ctx.vkCommandPool));
}

static void createDescriptorPool(AppContext& ctx) {
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

    vkCheck(vkCreateDescriptorPool(ctx.vkDevice, &poolInfo, nullptr, &ctx.vkDescriptorPool));
}



}
