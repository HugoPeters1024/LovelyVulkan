#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct Buffer {
    VkBuffer buffer;
    VmaAllocation memory;
};

struct MappedBuffer : public Buffer {
    void* data;

    template<typename T>
    inline T* getData() { return reinterpret_cast<T*>(data); }
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
