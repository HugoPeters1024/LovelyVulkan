#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct Image {
    VkFormat format;
    VkImage image;
    VmaAllocation allocation;
    VkImageView view;
};


namespace imagetools {
    void create_image_D(AppContext& ctx, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, VkImageLayout imageLayout, Image* dst);

    void load_image_D(AppContext& ctx, VkImageLayout initialLayout, const char* filename, Image* dst);

    void destroyImage(AppContext& ctx, Image& image);
}

}
