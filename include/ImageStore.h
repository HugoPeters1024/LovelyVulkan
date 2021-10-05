#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "ImageTools.h"
#include "Window.h"

namespace lv {

enum class ImageID : uint32_t;

struct ImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageLayout initialLayout;
    FrameSelector<uint32_t> width;
    FrameSelector<uint32_t> height;
};


struct ImageStoreInfo {
    std::unordered_map<ImageID, ImageInfo> m_imageInfos;
    std::unordered_map<ImageID, ImageInfo> m_staticImageInfos;

    inline void defineImage(ImageID slot, VkFormat format, VkImageUsageFlags usage, VkImageLayout initialLayout) {
        ImageInfo info { format, usage, initialLayout };
        m_imageInfos.insert({slot, info});
    }

    inline void defineStaticImage(ImageID slot, VkFormat format, VkImageUsageFlags usage, VkImageLayout initialLayout) {
        ImageInfo info { format, usage, initialLayout };
        m_staticImageInfos.insert({slot, info});
    }
};

struct ImageStoreFrame : public FrameExt {
    std::unordered_map<ImageID, Image> images;
    std::unordered_map<ImageID, Image*> staticImages;

    inline const Image& get(ImageID slot) const {
        assert(images.find(slot) != images.end() && "Image not registered before use");
        return images.at(slot);
    }

    inline const Image* getStatic(ImageID slot) const {
        assert(staticImages.find(slot) != staticImages.end() && "Image not registered before use");
        return staticImages.at(slot);
    }
};


class ImageStore : public AppExt {
public:
    ImageStore(AppContext& ctx, ImageStoreInfo info);
    ~ImageStore();

    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;
private:
    Image createImage(AppContext& ctx, uint32_t width, uint32_t height, ImageInfo& info);
    ImageStoreInfo info;
    std::unordered_map<ImageID, Image> staticImages;
};
}
