#include "ImageStore.h"

namespace lv {

ImageStoreFrame* ImageStore::buildFrame(FrameContext& frame) {
    auto ret = new ImageStoreFrame();
    auto imageInfo = vks::initializers::imageCreateInfo(frame.swapchain.width, frame.swapchain.height, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT);
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY } ;
    vkCheck(vmaCreateImage(frame.ctx.vmaAllocator, &imageInfo, &allocInfo, &ret->image, &ret->allocation, nullptr));

    auto viewInfo = vks::initializers::imageViewCreateInfo(ret->image, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCheck(vkCreateImageView(frame.ctx.vkDevice, &viewInfo, nullptr, &ret->view));
    return ret;
}

void ImageStore::destroyFrame(AppContext& ctx, ImageStoreFrame* frame) {
    vkDestroyImageView(ctx.vkDevice, frame->view, nullptr);
    vmaDestroyImage(ctx.vmaAllocator, frame->image, frame->allocation);
}
 
}
