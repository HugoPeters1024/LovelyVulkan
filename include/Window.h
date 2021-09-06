#pragma once
#include "precomp.h"


namespace lv {

class AppContext;

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct FrameContext {
    AppContext& ctx;
    struct {
        VkImage vkImage;
        VkImageView vkView;
        VkFormat format;
        uint32_t width;
        uint32_t height;
    } swapchain;
    VkCommandBuffer commandBuffer;
    FrameContext* fNext;
    FrameContext* fPrev;

    std::unordered_map<std::type_index, void*> extensionFrame; 

    FrameContext(AppContext& ctx) : ctx(ctx) {}

    template<typename T>
    T& getExtFrame() {
        return *reinterpret_cast<T*>(extensionFrame[typeid(T)]);
    }

    void* getExtFrame(std::type_index typeName) {
        return extensionFrame[typeName];
    }
};

struct WindowInfo {
    std::string windowName;
    uint32_t width;
    uint32_t height;
};


class Window : NoCopy {
public:
    AppContext& ctx;
    WindowInfo info;
    int32_t width, height;
    GLFWwindow* glfwWindow;
    VkSurfaceKHR vkSurface;
    bool wasResized = false;

    struct {
        uint32_t currentInFlight = 0;
        uint32_t imageIdx = 0;
        VkSwapchainKHR vkSwapchain = VK_NULL_HANDLE;
        VkSwapchainKHR vkOldSwapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        std::vector<FrameContext> frameContexts;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> imagesInFlight;
        std::vector<VkFence> inFlightFences;
    } swapchain;

public:
    Window(AppContext& ctx, WindowInfo info);
    ~Window();

    bool shouldClose() const;
    void nextFrame(std::function<void(FrameContext&)> callback);

    inline std::vector<FrameContext>& getAllFrames() { return swapchain.frameContexts; }
    inline uint32_t getNrFrames() const { return swapchain.frameContexts.size(); }
    inline uint32_t getFrameID() const { return swapchain.imageIdx; }


private:
    void createWindow(const char* name, int32_t width, int32_t height);
    void createSwapchain();
    void buildExtensionFrames();
    void recreateSwapchain();

    SwapchainSupport querySwapchainSupport() const;
    VkSurfaceFormatKHR chooseSurfaceFormat(const SwapchainSupport& support) const;
    VkPresentModeKHR choosePresentMode(const SwapchainSupport& support) const;
    VkExtent2D chooseExtent(const SwapchainSupport& support) const;
};

}


