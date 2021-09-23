#include "BufferTools.h"

namespace lv {

namespace buffertools {

void createH2D_buffer(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst) {
    auto bufferInfo = vks::initializers::bufferCreateInfo(usage, static_cast<VkDeviceSize>(size));
    VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &dst->buffer, &dst->memory, nullptr));
}

void createH2D_buffer_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst) {
    createH2D_buffer(ctx, usage, size, dst);

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
