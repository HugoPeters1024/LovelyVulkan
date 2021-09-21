#include "RayTracer.h"

namespace lv {

RayTracer::RayTracer(AppContext& ctx, RayTracerInfo info) : AppExt<RayTracerFrame>(ctx), info(info) {
    loadFunctions();
    createBottomLevelAccelerationStructure();
    createTopLevelAccelerationStructure();
}

RayTracer::~RayTracer() {
}

RayTracerFrame* RayTracer::buildFrame(FrameContext& frame) {
    return new RayTracerFrame {};
}

void RayTracer::destroyFrame(RayTracerFrame* frame) {
}

void RayTracer::loadFunctions() {
    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkGetBufferDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(ctx.vkDevice, "vkCreateRayTracingPipelinesKHR"));

}

RayTracingScratchBuffer RayTracer::createScratchBuffer(VkDeviceSize size) {
    RayTracingScratchBuffer scratchBuffer{};

    auto bufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, size);
    auto allocCreateInfo = VmaAllocationCreateInfo { 
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocCreateInfo, &scratchBuffer.handle, &scratchBuffer.memory, nullptr));

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = scratchBuffer.handle,
    };
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddress(ctx.vkDevice, &bufferDeviceAddressInfo);
    return scratchBuffer;
}

void RayTracer::destroyScratchBuffer(RayTracingScratchBuffer& scratchBuffer) {
    vmaDestroyBuffer(ctx.vmaAllocator, scratchBuffer.handle, scratchBuffer.memory);
}

AccelerationStructure RayTracer::createAccelerationStructureBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
    AccelerationStructure ret{};
    auto bufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, buildSizeInfo.accelerationStructureSize);
    auto allocCreateInfo = VmaAllocationCreateInfo { 
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocCreateInfo, &ret.buffer, &ret.memory, nullptr));
    return ret;
}

uint64_t RayTracer::getBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfoKHR info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddress(ctx.vkDevice, &info);
}

void RayTracer::createBottomLevelAccelerationStructure() {
    struct Vertex { float pos[3]; };

    std::array<Vertex, 3> vertices {
        Vertex { .pos = {  1.0f,  1.0f, 0.0f }},
        Vertex { .pos = { -1.0f,  1.0f, 0.0f }},
        Vertex { .pos = {  0.0f, -1.0f, 0.0f }},
    };

    std::array<uint32_t, 3> indices = { 0, 1, 2 };

    VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };


    {
        auto vertexBufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, vertices.size() * sizeof(Vertex));
        auto vertexAllocInfo = VmaAllocationCreateInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
        vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &vertexBufferInfo, &vertexAllocInfo, &vertexBuffer, &vertexBufferMemory, nullptr));
        void* vertexData;
        vkCheck(vmaMapMemory(ctx.vmaAllocator, vertexBufferMemory, &vertexData));
        memcpy(vertexData, vertices.data(), vertices.size() * sizeof(Vertex));
        vmaUnmapMemory(ctx.vmaAllocator, vertexBufferMemory);
    }

    {
        auto indexBufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, indices.size() * sizeof(uint32_t));
        auto indexAllocInfo = VmaAllocationCreateInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
        vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &indexBufferInfo, &indexAllocInfo, &indexBuffer, &indexBufferMemory, nullptr));
        void* indexData;
        vkCheck(vmaMapMemory(ctx.vmaAllocator, indexBufferMemory, &indexData));
        memcpy(indexData, indices.data(), indices.size() * sizeof(uint32_t));
        vmaUnmapMemory(ctx.vmaAllocator, indexBufferMemory);
    }

    {
        auto transformBufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, sizeof(VkTransformMatrixKHR));
        auto transformAllocInfo = VmaAllocationCreateInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
        vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &transformBufferInfo, &transformAllocInfo, &transformBuffer, &transformBufferMemory, nullptr));
        void* transformData;
        vkCheck(vmaMapMemory(ctx.vmaAllocator, transformBufferMemory, &transformData));
        memcpy(transformData, &transformMatrix, sizeof(VkTransformMatrixKHR));
        vmaUnmapMemory(ctx.vmaAllocator, transformBufferMemory);
    }

    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(vertexBuffer);
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(indexBuffer);
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(transformBuffer);

    // The actual build
    VkAccelerationStructureGeometryTrianglesDataKHR triangleData {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData = vertexBufferDeviceAddress,
        .vertexStride = sizeof(Vertex),
        .maxVertex = 3,
        .indexType = VK_INDEX_TYPE_UINT32,
        .indexData = indexBufferDeviceAddress,
        .transformData = transformBufferDeviceAddress,
    };

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles = triangleData;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    const uint32_t numTriangles = 1;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo);

    bottomAC = createAccelerationStructureBuffer(accelerationStructureBuildSizesInfo);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = bottomAC.buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCheck(vkCreateAccelerationStructureKHR(ctx.vkDevice, &accelerationStructureCreateInfo, nullptr, &bottomAC.handle));

    auto scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = bottomAC.handle;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;

    auto rangeInfoPtr = &accelerationStructureBuildRangeInfo;

    auto cmdBuffer = ctx.singleTimeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &accelerationStructureBuildGeometryInfo, &rangeInfoPtr);

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = bottomAC.handle;
    bottomAC.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(ctx.vkDevice, &accelerationDeviceAddressInfo);
    ctx.endSingleTimeCommands(cmdBuffer);

    destroyScratchBuffer(scratchBuffer);
}

void RayTracer::createTopLevelAccelerationStructure() {
    VkTransformMatrixKHR transformMatrix {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };

    VkAccelerationStructureInstanceKHR instance{};
    instance.transform = transformMatrix;
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = bottomAC.deviceAddress;

    VkBuffer instanceBuffer;
    VmaAllocation instanceBufferMemory;
    {
        auto bufferInfo = vks::initializers::bufferCreateInfo(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, sizeof(VkAccelerationStructureInstanceKHR));
        VmaAllocationCreateInfo allocInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
        vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &instanceBuffer, &instanceBufferMemory, nullptr));
        void* data;
        vkCheck(vmaMapMemory(ctx.vmaAllocator, instanceBufferMemory, &data));
        memcpy(data, &instance, sizeof(VkAccelerationStructureInstanceKHR));
        vmaUnmapMemory(ctx.vmaAllocator, instanceBufferMemory);
    }

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instanceBuffer);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

    // Get the size info
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &accelerationStructureGeometry;

    uint32_t primitiveCount = 1;

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &buildSizesInfo);

    topAC = createAccelerationStructureBuffer(buildSizesInfo);

    VkAccelerationStructureCreateInfoKHR ASInfo{};
    ASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    ASInfo.buffer = topAC.buffer;
    ASInfo.size = buildSizesInfo.accelerationStructureSize;
    ASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCheck(vkCreateAccelerationStructureKHR(ctx.vkDevice, &ASInfo, nullptr, &topAC.handle));

    auto scratchBuffer = createScratchBuffer(buildSizesInfo.buildScratchSize);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = topAC.handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;
 
    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = 1;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &accelerationStructureBuildRangeInfo;

    auto cmdBuffer = ctx.singleTimeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &accelerationBuildGeometryInfo, &buildRangeInfoPtr);
    ctx.endSingleTimeCommands(cmdBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = topAC.handle;
    topAC.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(ctx.vkDevice, &addressInfo);

    destroyScratchBuffer(scratchBuffer);
    vmaDestroyBuffer(ctx.vmaAllocator, instanceBuffer, instanceBufferMemory);
}


}
