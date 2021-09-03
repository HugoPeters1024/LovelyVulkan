#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct ImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
};

struct Image {
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
};

typedef uint32_t ImageID;
struct ImageStoreFrame {
    std::unordered_map<ImageID, Image> images;

    inline Image& get(ImageID id) { return images[id]; }
};

class ImageStore : public AppExt<ImageStoreFrame> {
private:
    std::unordered_map<ImageID, ImageInfo> m_imageInfos;
public:
    inline void addImage(ImageID id, VkFormat format, VkImageUsageFlags usage) {
        ImageInfo info { format, usage };
        m_imageInfos.insert({id, info});
    }

    void buildCore(AppContext& ctx) override {}
    void destroyCore(AppContext& ctx) override {}

    ImageStoreFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(AppContext& ctx, ImageStoreFrame* frame) override;
};

}
