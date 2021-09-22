#include "RayTracer.h"
#include "ImageStore.h"

namespace lv {

RayTracer::RayTracer(AppContext& ctx, RayTracerInfo info) : AppExt<RayTracerFrame>(ctx), info(info) {
    loadFunctions();
    getFeatures();
    createRayTracingPipeline();
    createShaderBindingTable();
    createBottomLevelAccelerationStructure();
    createTopLevelAccelerationStructure();
}

RayTracer::~RayTracer() {
}

RayTracerFrame* RayTracer::buildFrame(FrameContext& frame) {
    auto ret = new RayTracerFrame();

    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(ctx.vkDescriptorPool, &descriptorSetLayout, 1);
    vkCheck(vkAllocateDescriptorSets(ctx.vkDevice, &allocInfo, &ret->descriptorSet));

    VkWriteDescriptorSetAccelerationStructureKHR writeASInfo{};
    writeASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    writeASInfo.accelerationStructureCount = 1;
    writeASInfo.pAccelerationStructures = &topAC.handle;

    VkWriteDescriptorSet writeAS{};
    writeAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeAS.pNext = &writeASInfo;
    writeAS.dstSet = ret->descriptorSet;
    writeAS.dstBinding = 0;
    writeAS.descriptorCount = 1;
    writeAS.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    auto imageFrame = frame.getExtFrame<lv::ImageStoreFrame>();

    VkDescriptorImageInfo imageDescriptorInfo{};
    imageDescriptorInfo.imageView = imageFrame.get(lv::ImageID(1)).view;
    imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    auto imageWrite = vks::initializers::writeDescriptorSet(ret->descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &imageDescriptorInfo);


    std::array<VkWriteDescriptorSet, 2> writes { writeAS, imageWrite };
    vkUpdateDescriptorSets(ctx.vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return ret;
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

uint64_t RayTracer::getBufferDeviceAddress(VkBuffer buffer) const {
    VkBufferDeviceAddressInfoKHR info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddress(ctx.vkDevice, &info);
}

void RayTracer::getFeatures() {
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(ctx.vkPhysicalDevice, &deviceProperties);

    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &accelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(ctx.vkPhysicalDevice, &deviceFeatures);
}

void RayTracer::createRayTracingPipeline() {
    auto ASLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0);
    auto resultImageLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1);
    auto uniformBufferBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 2);
    std::vector<VkDescriptorSetLayoutBinding> bindings { ASLayoutBinding, resultImageLayoutBinding, uniformBufferBinding };

    auto layoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(bindings);
    vkCheck(vkCreateDescriptorSetLayout(ctx.vkDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout));

    auto pipelineLayoutInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout);
    vkCheck(vkCreatePipelineLayout(ctx.vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout));

    // Setup ray tracing shader groups
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    {
        // Ray gen 
        shaderStages.push_back(vks::initializers::pipelineShaderStageCreateInfo(vks::tools::loadShader("./app/shaders_bin/raygen.rgen.spv", ctx.vkDevice), VK_SHADER_STAGE_RAYGEN_BIT_KHR));
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(shaderGroup);
    }

    {
        // Miss group
        shaderStages.push_back(vks::initializers::pipelineShaderStageCreateInfo(vks::tools::loadShader("./app/shaders_bin/miss.rmiss.spv", ctx.vkDevice), VK_SHADER_STAGE_MISS_BIT_KHR));
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(shaderGroup);
    }

    {
        // Closest hit
        shaderStages.push_back(vks::initializers::pipelineShaderStageCreateInfo(vks::tools::loadShader("./app/shaders_bin/closesthit.rchit.spv", ctx.vkDevice), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(shaderGroup);
    }

    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
    rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayTracingPipelineCI.pStages = shaderStages.data();
    rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
    rayTracingPipelineCI.pGroups = shaderGroups.data();
    rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineCI.layout = pipelineLayout;
    vkCheck(vkCreateRayTracingPipelinesKHR(ctx.vkDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));
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

void RayTracer::createShaderBindingTable() {
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
    const uint32_t sbtSize = groupCount * handleSizeAlligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vkCheck(vkGetRayTracingShaderGroupHandlesKHR(ctx.vkDevice, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto bufferInfo = vks::initializers::bufferCreateInfo(bufferUsageFlags, static_cast<VkDeviceSize>(handleSize));
    auto allocInfo = VmaAllocationCreateInfo { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &raygenShaderBindingTable, &raygenShaderBindingTableMemory, nullptr));
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &missShaderBindingTable, &missShaderBindingTableMemory, nullptr));
    vkCheck(vmaCreateBuffer(ctx.vmaAllocator, &bufferInfo, &allocInfo, &hitShaderBindingTable, &hitShaderBindingTableMemory, nullptr));

    void* data;
    vkCheck(vmaMapMemory(ctx.vmaAllocator, raygenShaderBindingTableMemory, &data));
    memcpy(data, shaderHandleStorage.data() + handleSizeAlligned * 0, handleSize);
    vmaUnmapMemory(ctx.vmaAllocator, raygenShaderBindingTableMemory);

    vkCheck(vmaMapMemory(ctx.vmaAllocator, missShaderBindingTableMemory, &data));
    memcpy(data, shaderHandleStorage.data() + handleSizeAlligned * 1, handleSize);
    vmaUnmapMemory(ctx.vmaAllocator, missShaderBindingTableMemory);

    vkCheck(vmaMapMemory(ctx.vmaAllocator, hitShaderBindingTableMemory, &data));
    memcpy(data, shaderHandleStorage.data() + handleSizeAlligned * 2, handleSize);
    vmaUnmapMemory(ctx.vmaAllocator, hitShaderBindingTableMemory);
}


void RayTracer::render(FrameContext& frame) const {
    const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(raygenShaderBindingTable);
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(missShaderBindingTable);
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(hitShaderBindingTable);
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &frame.getExtFrame<RayTracerFrame>().descriptorSet, 0, 0);

    vkCmdTraceRaysKHR(frame.commandBuffer, &raygenShaderSbtEntry, &missShaderSbtEntry, &hitShaderSbtEntry, &callableShaderSbtEntry,frame.swapchain.width, frame.swapchain.height,1);
}

}
