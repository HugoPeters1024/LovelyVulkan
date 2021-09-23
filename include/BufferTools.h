#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct Buffer {
    VkBuffer buffer;
    VmaAllocation memory;
};

namespace buffertools {
    void createH2D_buffer(AppContext& ctx, VkBufferUsageFlags usage, size_t size, Buffer* dst);
    void createH2D_buffer_data(AppContext& ctx, VkBufferUsageFlags usage, size_t size, void* data, Buffer* dst);
    void destroyBuffer(AppContext& ctx, Buffer& buffer);
}
}
