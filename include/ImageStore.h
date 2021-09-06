#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

enum class ImageID : uint32_t;

struct ImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageLayout initialLayout;
};

struct Image {
    VkFormat format;
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
};

struct ImageStoreInfo {
    std::unordered_map<ImageID, ImageInfo> m_imageInfos;

    inline void defineImage(ImageID slot, VkFormat format, VkImageUsageFlags usage, VkImageLayout initialLayout) {
        ImageInfo info { format, usage, initialLayout };
        m_imageInfos.insert({slot, info});
    }
};

struct ImageStoreFrame {
    std::unordered_map<ImageID, Image> images;

    inline Image& get(ImageID slot) {
        assert(images.find(slot) != images.end() && "Image not registered before use");
        return images[slot];
    }
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
