#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct ImageStoreFrame {
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
};

class ImageStore : public AppExt<ImageStoreFrame> {
public:
    void buildCore(AppContext& ctx) override {}
    void destroyCore(AppContext& ctx) override {}

    ImageStoreFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(AppContext& ctx, ImageStoreFrame* frame) override;
};

}
