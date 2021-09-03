#include "ImageStore.h"

namespace lv {

ImageStoreFrame* ImageStore::buildFrame(FrameContext& frame) {
    auto ret = new ImageStoreFrame();

    for(auto& pair : m_imageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        Image img{};
        auto imageCreateInfo = vks::initializers::imageCreateInfo(frame.swapchain.width, frame.swapchain.height, imageInfo.format, imageInfo.usage);
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY } ;
        vkCheck(vmaCreateImage(frame.ctx.vmaAllocator, &imageCreateInfo, &allocInfo, &img.image, &img.allocation, nullptr));

        auto viewInfo = vks::initializers::imageViewCreateInfo(img.image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(frame.ctx.vkDevice, &viewInfo, nullptr, &img.view));

        ret->images.insert({imageIdx, img});
    }
    return ret;
}

void ImageStore::destroyFrame(AppContext& ctx, ImageStoreFrame* frame) {
    for(auto& pair : frame->images) {
        auto& image = pair.second;
        vkDestroyImageView(ctx.vkDevice, image.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, image.image, image.allocation);
    }
}
 
}
