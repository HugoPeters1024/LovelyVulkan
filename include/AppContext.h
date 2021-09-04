#pragma once
#include "precomp.h"
#include "AppExt.h"

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
    std::vector<std::type_index> extensionOrder;
    std::unordered_map<std::type_index, IAppExt*> extensions;

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

    template<class T, typename... Args>
    T* registerExtension(Args&&... args) {
        static_assert(std::is_base_of<IAppExt, T>::value, "Extensions must be derived from IAppExt");
        T* ext = new T(*this, std::forward<Args>(args)...);
        extensions.insert({typeid(T), ext});
        extensionOrder.emplace_back(typeid(T));
        return ext;
    }

    template<class T>
    T* getExtension() {
        static_assert(std::is_base_of<IAppExt, T>::value, "Extensions must be derived from IAppExt");
        return reinterpret_cast<T*>(extensions[typeid(T)]);
    }

private:

    // Will be invalid after construction
    struct {
        GLFWwindow* window;
        VkSurfaceKHR surface;
    } windowHelper;

public:
    AppContext(AppContextInfo info);
    ~AppContext();

    VkShaderModule createShaderModule(const char* filePath) const;

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

};
