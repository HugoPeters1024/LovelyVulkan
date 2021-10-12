#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "FrameManager.h"

namespace lv {

struct OverlayInfo {
    VkRenderPass renderPass;
    GLFWwindow* glfwWindow;
    FrameManager* frameManager;
};

class Overlay : public AppExt {
public:
    Overlay(AppContext& ctx, OverlayInfo info);
    ~Overlay();

    void render(FrameContext& frameContext, float energy, float fps);

    bool NEE = false;
private:
    void createDescriptorPool();
    void initImgui();

    OverlayInfo info;
    VkDescriptorPool imguiPool;

};

}
