#include "ImageStore.h"

namespace lv {

ImageStore::ImageStore(AppContext& ctx, ImageStoreInfo info) 
   : AppExt(ctx), info(info) {
    for(auto& pair : info.m_staticImageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        // TODO: no hardcoding of resolution
        Image img = createImage(ctx, 1280, 768, imageInfo);
        staticImages.insert({imageIdx, img});
    }
}

ImageStore::~ImageStore() {
    for(const auto& pair : staticImages) {
        vkDestroyImageView(ctx.vkDevice, pair.second.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, pair.second.image, pair.second.allocation);
    }
}

void ImageStore::embellishFrameContext(FrameContext& frame) {
    auto& imageFrame = frame.registerExtFrame<ImageStoreFrame>();

    for(auto& pair : info.m_imageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        Image img = createImage(ctx, imageInfo.width(frame), imageInfo.height(frame), imageInfo);
        imageFrame.images.insert({imageIdx, img});
    }

    for(auto& pair : info.m_staticImageInfos) {
        auto& imageIdx = pair.first;
        imageFrame.staticImages.insert({imageIdx, &staticImages[imageIdx]});
    }
}

void ImageStore::cleanupFrameContext(FrameContext& frame) {
    auto& imageFrame = frame.getExtFrame<ImageStoreFrame>();

    for(auto& pair : imageFrame.images) {
        auto& image = pair.second;
        vkDestroyImageView(ctx.vkDevice, image.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, image.image, image.allocation);
    }
}

Image ImageStore::createImage(AppContext& ctx, uint32_t width, uint32_t height, ImageInfo& info) {
    Image ret{};
    ret.format = info.format;
    auto imageCreateInfo = vks::initializers::imageCreateInfo(width, height, info.format, info.usage);
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY } ;
    vkCheck(vmaCreateImage(ctx.vmaAllocator, &imageCreateInfo, &allocInfo, &ret.image, &ret.allocation, nullptr));

    auto viewInfo = vks::initializers::imageViewCreateInfo(ret.image, info.format, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCheck(vkCreateImageView(ctx.vkDevice, &viewInfo, nullptr, &ret.view));


    auto cmdBuffer = ctx.singleTimeCommandBuffer();
    auto barrier = vks::initializers::imageMemoryBarrier(
            ret.image,
            VK_IMAGE_LAYOUT_UNDEFINED, info.initialLayout);
    vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr,
            0, nullptr,
            1, &barrier);

    ctx.endSingleTimeCommands(cmdBuffer);
    return ret;
}
 
}
