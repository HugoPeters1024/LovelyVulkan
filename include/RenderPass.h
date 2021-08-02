#include "precomp.h"
#include "Device.h"
#include "Window.h"

namespace lv {

struct AttachmentView {
    VkFormat* format;
    uint32_t* binding;
    std::vector<VkImage>* images;
    std::vector<VkImageView>* imageViews;
};

struct ColorAttachment {
    VkFormat format;
    uint32_t binding;
    std::vector<VkImage> images;
    std::vector<VmaAllocation> imagesMemory;
    std::vector<VkImageView> imageViews;

    inline AttachmentView createView() {
        return AttachmentView {
            .format = &format,
            .binding = &binding,
            .images = &images,
            .imageViews = &imageViews,
        };
    }
};

class RenderPassCore : NoCopy {
private:
    Device& device;

    // Optional pointer to a window to resize accordingly
    Window* parent;

    // Dimensions and duplication, ignored if parent is set
    uint32_t width, height, duplication;

    std::vector<ColorAttachment> attachments;

    // Resulting pass
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;

    void finalizeState();
    void createAttachments();
    void createRenderPass();
    void createFramebuffers();

public:
    RenderPassCore(Device& device);
    ~RenderPassCore();
    inline RenderPassCore& setDimensions(uint32_t w, uint32_t h) { width = w; height = h; return *this; }
    inline RenderPassCore& setDuplication(uint32_t d) { duplication = d; return *this; }
    inline RenderPassCore& bindToWindow(Window* window) { parent = window; return *this; }
    inline RenderPassCore& addAttachment(AttachmentView* view, VkFormat format, uint32_t binding) {
        attachments.push_back(ColorAttachment {
            .format = format,
            .binding = binding,
        });
        *view = attachments.back().createView();
        return *this;
    }

    void build();
};

}