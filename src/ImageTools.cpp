#include "ImageTools.h"
#include "BufferTools.h"

namespace lv {

namespace imagetools {
    void create_image_D(AppContext& ctx, uint32_t width, uint32_t height, VkImageUsageFlags usage, VkFormat format, VkImageLayout initialLayout, Image* dst) {
        dst->format = format;
        auto imageCreateInfo = vks::initializers::imageCreateInfo(width, height, format, usage);
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
        vkCheck(vmaCreateImage(ctx.vmaAllocator, &imageCreateInfo, &allocInfo, &dst->image, &dst->allocation, nullptr));

        auto viewInfo = vks::initializers::imageViewCreateInfo(dst->image, format, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCheck(vkCreateImageView(ctx.vkDevice, &viewInfo, nullptr, &dst->view));

        auto cmdBuffer = ctx.singleTimeCommandBuffer();
        auto barrier = vks::initializers::imageMemoryBarrier(dst->image, VK_IMAGE_LAYOUT_UNDEFINED, initialLayout);
    vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        ctx.endSingleTimeCommands(cmdBuffer);
    }

    void load_image_D(AppContext& ctx, VkImageLayout initialLayout, const char* filename, Image* dst) {
        int width, height, nrChannels;
        stbi_uc* pixels = stbi_load(filename, &width, &height, &nrChannels, STBI_rgb_alpha);
        if (!pixels) {
            logger::error("Could not load image {}", filename);
            exit(1);
        }

        assert(nrChannels == 4 && "Image loading should produce a 4 channel buffer");


        // We are loading as 4 bytes per pixel
        VkDeviceSize imageSize = width * height * 4;
        Buffer stagingBuffer;
        buffertools::create_buffer_H(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize, &stagingBuffer);

        void* data;
        vkCheck(vmaMapMemory(ctx.vmaAllocator, stagingBuffer.memory, &data));
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vmaUnmapMemory(ctx.vmaAllocator, stagingBuffer.memory);
        vmaFlushAllocation(ctx.vmaAllocator, stagingBuffer.memory, 0, imageSize);

        create_image_D(ctx, width, height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst);

        auto cmdBuffer = ctx.singleTimeCommandBuffer();
        VkBufferImageCopy copyRegion = vks::initializers::imageCopy(width, height);
        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        auto barrier = vks::initializers::imageMemoryBarrier(dst->image, VK_IMAGE_LAYOUT_UNDEFINED, initialLayout);
    vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        ctx.endSingleTimeCommands(cmdBuffer);

        buffertools::destroyBuffer(ctx, stagingBuffer);
    }

    void destroyImage(AppContext& ctx, Image& image) {
        vkDestroyImageView(ctx.vkDevice, image.view, nullptr);
        vmaDestroyImage(ctx.vmaAllocator, image.image, image.allocation);
    }
}



}
