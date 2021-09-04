#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct ImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
};

struct Image {
    VkFormat format;
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
};

typedef uint32_t ImageID;
struct ImageStoreInfo {
    std::unordered_map<ImageID, ImageInfo> m_imageInfos;
    inline void defineImage(ImageID slot, VkFormat format, VkImageUsageFlags usage) {
        ImageInfo info { format, usage };
        m_imageInfos.insert({slot, info});
    }
};

struct ImageStoreFrame {
    std::unordered_map<ImageID, Image> images;
    inline Image& get(ImageID slot) { return images[slot]; }
};


class ImageStore : public AppExt<ImageStoreFrame> {
public:
    ImageStore(AppContext& ctx, ImageStoreInfo info);
    ~ImageStore();

    ImageStoreFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(AppContext& ctx, ImageStoreFrame* frame) override;
private:
    ImageStoreInfo info;
};
}
