#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct Buffer {
    VkBuffer buffer;
    VmaAllocation memory;
};

namespace buffertools {
    void create_buffer_H2D(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst);
    void create_buffer_H2D_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst);

    void create_buffer_D(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst);
    void create_buffer_D_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst);

    void create_buffer_H(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst);
    void create_buffer_H_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst);

    void destroyBuffer(AppContext& ctx, Buffer& buffer);
}
}
