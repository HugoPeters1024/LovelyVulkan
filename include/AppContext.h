#pragma once
#include "precomp.h"

namespace lv {
    struct AppContextInfo {
        uint32_t apiVersion = VK_API_VERSION_1_2;
        std::set<const char*> validationLayers;
        std::set<const char*> instanceExtensions;
        std::set<const char*> deviceExtensions;
    };

    class AppContext : NoCopy {
    public:
        AppContextInfo info;
        VkInstance vkInstance;
        VkPhysicalDevice vkPhysicalDevice;
        VkDevice vkDevice;
        VmaAllocator vmaAllocator;

        struct {
            std::optional<uint32_t> compute, graphics, present;
        } queueFamilies;

        struct {
            VkQueue compute, graphics, present;
        } queues;

        VkCommandPool vkCommandPool;
        VkDescriptorPool vkDescriptorPool;

        struct {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        } swapchainSupport;

    private:

    // Will be invalid after construction
    struct {
        GLFWwindow* window;
        VkSurfaceKHR surface;
    } windowHelper;

    public:
        AppContext(AppContextInfo info);
        ~AppContext();

    private:
        void initWindowingSystem();
        void finalizeInfo();
        void createInstance();
        void createWindowSurface();
        void pickPhysicalDevice();
        void findQueueFamilies();
        void createLogicalDevice();
        void cleanupWindowHelper();
        void createVmaAllocator();
        void createCommandPool();
        void createDescriptorPool();


    };
}

