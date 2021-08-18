#include "precomp.h"
#include "Device.h"
#include "Window.h"
#include "Structs.hpp"

namespace lv {


class RenderPassCore : NoCopy {
private:
    Device& device;

    // Optional pointer to a window to resize accordingly
    Window* parent;

    // Dimensions and duplication, ignored if parent is set
    uint32_t width, height, duplication;

    std::vector<ColorAttachment> attachments;
    std::vector<AttachmentView> attachmentViews;

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
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
        attachmentViews.push_back(attachments.back().createView());
        *view = attachmentViews.back();
        return *this;
    }
    inline RenderPassCore& addWindowAttachment(AttachmentView* view, Window* window, uint32_t binding) {
        attachmentViews.push_back(window->getColorAttachmentView(binding));
        *view = attachmentViews.back();
        return *this;
    }

    void build();
};

}