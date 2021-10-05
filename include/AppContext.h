#pragma once
#include "precomp.h"

namespace lv {

class AppExt;
class AppContext;
class AppContextInfo;
class FrameManager;

template<typename T>
struct app_extensions {
    static_assert(std::is_base_of<AppExt, T>::value, "Extensions must be derived from AppExt");
    void operator()(AppContextInfo& ctx) const {}
};

struct AppContextInfo {
    uint32_t apiVersion = VK_API_VERSION_1_2;
    std::set<const char*> validationLayers;
    std::set<const char*> instanceExtensions;
    std::set<const char*> deviceExtensions;

    std::set<std::type_index> knownExtensions;

    template<class T, typename... Args>
    void registerExtension(Args&&... args) {
        static_assert(std::is_base_of<AppExt, T>::value, "Extensions must be derived from AppExt");
        logger::info("Registered extension {}", typeid(T).name());
        app_extensions<T>()(*this);
        knownExtensions.insert(typeid(T));
    }
};

class AppContext : NoCopy {
public:
    AppContextInfo info;
    VkInstance vkInstance;
    VkPhysicalDevice vkPhysicalDevice;
    VkDevice vkDevice;
    VmaAllocator vmaAllocator;
    std::vector<std::type_index> extensionOrder;
    std::unordered_map<std::type_index, AppExt*> extensions;
    std::vector<FrameManager*> frameManagers;

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

    bool extensionRegistered(std::type_index type) {
        return info.knownExtensions.find(type) != info.knownExtensions.end();
    }

    template<typename T, typename... Args>
    T& addExtension(Args&&... args) {
        static_assert(std::is_base_of<AppExt, T>::value, "Extensions must be derived from AppExt");
        assert(info.knownExtensions.find(typeid(T)) != info.knownExtensions.end() && "Extension used without registering");
        extensionOrder.push_back(typeid(T));
        extensions.insert({typeid(T), new T(std::forward<Args>(args)...)});
        return *reinterpret_cast<T*>(extensions[typeid(T)]);
    }

    template<typename T, typename... Args>
    T& addFrameManager(Args&&... args) {
        static_assert(std::is_base_of<FrameManager, T>::value, "Frame managers must be derived from FrameManager");
        frameManagers.push_back(new T(std::forward<Args>(args)...));
        auto& ret = *reinterpret_cast<T*>(frameManagers.back());
        std::vector<AppExt*> extensionPtrs;
        for(const auto& extType : extensionOrder) {
            extensionPtrs.push_back(extensions[extType]);
        }
        ret.init(extensionPtrs);
        return ret;
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
    VkCommandBuffer singleTimeCommandBuffer() const;
    void endSingleTimeCommands(VkCommandBuffer cmdBuffer) const;

private:
    void initWindowingSystem();
    void finalizeInfo();
    void createInstance();
    void createWindowSurface();
    void pickPhysicalDevice();
    void findQueueFamilies();
    void createLogicalDevice();
    void cleanupWindowHelper() const;
    void createVmaAllocator();
    void createCommandPool();
    void createDescriptorPool();
};

template<typename T>
using AppContextSelector = std::function<T(AppContext& ctx)>;

};
