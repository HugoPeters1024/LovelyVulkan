#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "ImageTools.h"
#include "BufferTools.h"
#include "FrameManager.h"

namespace lv {

struct ImageInfo {
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageLayout initialLayout;
    FrameSelector<uint32_t> width;
    FrameSelector<uint32_t> height;
};

struct BufferInfo {
    VkBufferUsageFlags usage;
    VkDeviceSize size;
};


struct ResourceStoreInfo {
    std::unordered_map<uint32_t, ImageInfo> m_imageInfos;
    std::unordered_map<uint32_t, ImageInfo> m_staticImageInfos;

    std::unordered_map<uint32_t, BufferInfo> m_bufferInfos;

    inline void defineImage(uint32_t slot, VkFormat format, VkImageUsageFlags usage, VkImageLayout initialLayout) {
        ImageInfo info { format, usage, initialLayout };
        m_imageInfos.insert({slot, info});
    }

    inline void defineStaticImage(uint32_t slot, VkFormat format, VkImageUsageFlags usage, VkImageLayout initialLayout) {
        ImageInfo info { format, usage, initialLayout };
        m_staticImageInfos.insert({slot, info});
    }

    inline void defineBuffer(uint32_t slot, VkBufferUsageFlags usage, VkDeviceSize size) {
        BufferInfo info { usage, size };
        m_bufferInfos.insert({slot, info});
    }
};

struct ResourceFrame : public FrameExt {
    std::unordered_map<uint32_t, Image> images;
    std::unordered_map<uint32_t, Image*> staticImages;

    std::unordered_map<uint32_t, MappedBuffer> buffers;

    inline const Image& get(uint32_t slot) const {
        assert(images.find(slot) != images.end() && "Image not registered before use");
        return images.at(slot);
    }

    inline const Image* getStatic(uint32_t slot) const {
        assert(staticImages.find(slot) != staticImages.end() && "Image not registered before use");
        return staticImages.at(slot);
    }

    inline MappedBuffer& getBuffer(uint32_t slot) {
        assert(buffers.find(slot) != buffers.end() && "Buffer not registered before use");
        return buffers[slot];
    }
};


class ResourceStore : public AppExt {
public:
    ResourceStore(AppContext& ctx, ResourceStoreInfo info);
    ~ResourceStore();

    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;
private:
    Image createImage(AppContext& ctx, uint32_t width, uint32_t height, ImageInfo& info);
    ResourceStoreInfo info;
    std::unordered_map<uint32_t, Image> staticImages;
};
}
