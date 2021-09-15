#include "ImageStore.h"

namespace lv {

ImageStore::ImageStore(AppContext& ctx, ImageStoreInfo info) 
   : AppExt<ImageStoreFrame>(ctx), info(info) {
}

ImageStore::~ImageStore() {
}

ImageStoreFrame* ImageStore::buildFrame(FrameContext& frame) {
    auto ret = new ImageStoreFrame();

    for(auto& pair : info.m_imageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        Image img {
            .format = imageInfo.format,
        };
        auto imageCreateInfo = vks::initializers::imageCreateInfo(frame.swapchain.width, frame.swapchain.height, imageInfo.format, imageInfo.usage);
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY } ;
        vkCheck(vmaCreateImage(frame.ctx.vmaAllocator, &imageCreateInfo, &allocInfo, &img.image, &img.allocation, nullptr));

        auto viewInfo = vks::initializers::imageViewCreateInfo(img.image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(frame.ctx.vkDevice, &viewInfo, nullptr, &img.view));


        auto cmdBuffer = ctx.singleTimeCommandBuffer();
        auto barrier = vks::initializers::imageMemoryBarrier(
                img.image,
                VK_IMAGE_LAYOUT_UNDEFINED, imageInfo.initialLayout);
        vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        ctx.endSingleTimeCommands(cmdBuffer);
        ret->images.insert({imageIdx, img});
    }
    return ret;
}

void ImageStore::destroyFrame(ImageStoreFrame* frame) {
    for(auto& pair : frame->images) {
        auto& image = pair.second;
        vkDestroyImageView(ctx.vkDevice, image.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, image.image, image.allocation);
    }
}
 
}
