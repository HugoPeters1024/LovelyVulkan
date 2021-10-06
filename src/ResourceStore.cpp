#include "ResourceStore.h"

namespace lv {

ResourceStore::ResourceStore(AppContext& ctx, ResourceStoreInfo info) 
   : AppExt(ctx), info(info) {
    for(auto& pair : info.m_staticImageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        // TODO: no hardcoding of resolution
        Image img = createImage(ctx, 1280, 768, imageInfo);
        staticImages.insert({imageIdx, img});
    }
}

ResourceStore::~ResourceStore() {
    for(const auto& pair : staticImages) {
        vkDestroyImageView(ctx.vkDevice, pair.second.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, pair.second.image, pair.second.allocation);
    }
}

void ResourceStore::embellishFrameContext(FrameContext& frame) {
    auto& rFrame = frame.registerExtFrame<ResourceFrame>();

    for(auto& pair : info.m_imageInfos) {
        auto& imageIdx = pair.first;
        auto& imageInfo = pair.second;

        Image img = createImage(ctx, imageInfo.width(frame), imageInfo.height(frame), imageInfo);
        rFrame.images.insert({imageIdx, img});
    }

    for(auto& pair : info.m_staticImageInfos) {
        auto& imageIdx = pair.first;
        rFrame.staticImages.insert({imageIdx, &staticImages[imageIdx]});
    }

    for(auto& pair : info.m_bufferInfos) {
        auto& bufferIdx = pair.first;
        auto& bufferInfo = pair.second;
        MappedBuffer buf;
        buffertools::create_buffer_H2D(ctx, bufferInfo.usage, bufferInfo.size, &buf);
        vkCheck(vmaMapMemory(ctx.vmaAllocator, buf.memory, &buf.data));
        rFrame.buffers.insert({bufferIdx, buf});
    }
}

void ResourceStore::cleanupFrameContext(FrameContext& frame) {
    auto& rFrame = frame.getExtFrame<ResourceFrame>();

    for(auto& pair : rFrame.buffers) {
        auto& buffer = pair.second;
        vmaUnmapMemory(ctx.vmaAllocator, buffer.memory);
        buffertools::destroyBuffer(ctx, buffer);
    }

    for(auto& pair : rFrame.images) {
        auto& image = pair.second;
        vkDestroyImageView(ctx.vkDevice, image.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, image.image, image.allocation);
    }
}

Image ResourceStore::createImage(AppContext& ctx, uint32_t width, uint32_t height, ImageInfo& info) {
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
