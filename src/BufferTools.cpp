#include "BufferTools.h"

namespace lv {

namespace buffertools {

void create_buffer_H2D(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst) {
    auto bufferInfo = vks::initializers::bufferCreateInfo(usage, static_cast<VkDeviceSize>(size));
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &dst->buffer, &dst->memory, nullptr));
}

void create_buffer_H2D_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst) {
    create_buffer_H2D(ctx, usage, size, dst);

    void* data_dst;
    vkCheck(vmaMapMemory(ctx.vmaAllocator, dst->memory, &data_dst));
    memcpy(data_dst, data, size);
    vmaUnmapMemory(ctx.vmaAllocator, dst->memory);
}

void create_buffer_D(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst) {
    auto bufferInfo = vks::initializers::bufferCreateInfo(usage, static_cast<VkDeviceSize>(size));
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &dst->buffer, &dst->memory, nullptr));
}

void create_buffer_D_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst) {
    Buffer staging;
    create_buffer_H_data(ctx, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size, data, &staging);

    create_buffer_D(ctx, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, size, dst);

    auto cmdBuffer = ctx.singleTimeCommandBuffer();
    VkBufferCopy copyRegion{};
    copyRegion.size = static_cast<VkDeviceSize>(size);
    vkCmdCopyBuffer(cmdBuffer, staging.buffer, dst->buffer, 1, &copyRegion);
    ctx.endSingleTimeCommands(cmdBuffer);

    buffertools::destroyBuffer(ctx, staging);
}

void create_buffer_H(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst) {
    auto bufferInfo = vks::initializers::bufferCreateInfo(usage, static_cast<VkDeviceSize>(size));
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_ONLY };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &dst->buffer, &dst->memory, nullptr));
}

void create_buffer_H_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst) {
    create_buffer_H(ctx, usage, size, dst);

    void* data_dst;
    vkCheck(vmaMapMemory(ctx.vmaAllocator, dst->memory, &data_dst));
    memcpy(data_dst, data, size);
    vmaUnmapMemory(ctx.vmaAllocator, dst->memory);
}

void destroyBuffer(AppContext& ctx, Buffer& buffer) {
    vmaDestroyBuffer(ctx.vmaAllocator, buffer.buffer, buffer.memory);
}

}
}
