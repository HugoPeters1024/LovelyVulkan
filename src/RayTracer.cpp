#include "RayTracer.h"

namespace lv {

RayTracer::RayTracer(AppContext& ctx, RayTracerInfo info) : AppExt(ctx), info(info) {
    loadFunctions();
    getFeatures();
    createRayTracingPipeline();
    createShaderBindingTable();
    createBottomLevelAccelerationStructures();
    createTopLevelAccelerationStructure();
    imagetools::load_image_D(ctx, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, "./app/bluenoise.png", &blueNoise);
}

RayTracer::~RayTracer() {
    imagetools::destroyImage(ctx, blueNoise);
    buffertools::destroyBuffer(ctx, vertexBuffer);
    buffertools::destroyBuffer(ctx, indexBuffer);
    buffertools::destroyBuffer(ctx, triangleDataBuffer);
    buffertools::destroyBuffer(ctx, transformBuffer);
    buffertools::destroyBuffer(ctx, raygenShaderBindingTable);
    buffertools::destroyBuffer(ctx, missShaderBindingTable);
    buffertools::destroyBuffer(ctx, hitShaderBindingTable);
    destroyAccelerationStructure(topAC);
    for(auto& as : bottomACs)
        destroyAccelerationStructure(as);


    vkDestroyPipeline(ctx.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.vkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.vkDevice, descriptorSetLayout, nullptr);
}

void RayTracer::embellishFrameContext(FrameContext& frame) {
    auto& ret = frame.registerExtFrame<RayTracerFrame>();
    auto& wFrame = frame.getExtFrame<WindowFrame>();

    // Camera uniform buffer
    glm::mat4 viewMatrix(1.0f);
    const float aspectRatio = (float)wFrame.width / wFrame.height;
    glm::mat4 projectionMatrix = glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f);

    RayTracerCamera camera {
        .viewInverse = glm::inverse(viewMatrix),
        .projInverse = glm::inverse(projectionMatrix),
    };

    buffertools::create_buffer_H2D_data(ctx, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(RayTracerCamera), &camera, &ret.cameraBuffer);

    // Blue noise sampler
    auto samplerInfo = vks::initializers::samplerCreateInfo(1.0f);
    vkCheck(vkCreateSampler(frame.ctx.vkDevice, &samplerInfo, nullptr, &ret.blueNoiseSampler));


    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(ctx.vkDescriptorPool, &descriptorSetLayout, 1);
    vkCheck(vkAllocateDescriptorSets(ctx.vkDevice, &allocInfo, &ret.descriptorSet));

    VkWriteDescriptorSetAccelerationStructureKHR writeASInfo{};
    writeASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    writeASInfo.accelerationStructureCount = 1;
    writeASInfo.pAccelerationStructures = &topAC.AShandle;

    VkWriteDescriptorSet writeAS{};
    writeAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeAS.pNext = &writeASInfo;
    writeAS.dstSet = ret.descriptorSet;
    writeAS.dstBinding = 0;
    writeAS.descriptorCount = 1;
    writeAS.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo imageDescriptorInfo{};
    imageDescriptorInfo.imageView = frame.getExtFrame<lv::ResourceFrame>().getStatic(1)->view;
    imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    auto imageWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &imageDescriptorInfo);

    VkDescriptorBufferInfo bufferDescriptorInfo{};
    bufferDescriptorInfo.buffer = ret.cameraBuffer.buffer;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet uniformBufferWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &bufferDescriptorInfo);

    VkDescriptorBufferInfo indexBufferDescriptorInfo{};
    indexBufferDescriptorInfo.buffer = indexBuffer.buffer;
    indexBufferDescriptorInfo.offset = 0;
    indexBufferDescriptorInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet indexBufferWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &indexBufferDescriptorInfo);

    VkDescriptorBufferInfo vertexBufferDescriptorInfo{};
    vertexBufferDescriptorInfo.buffer = vertexBuffer.buffer;
    vertexBufferDescriptorInfo.offset = 0;
    vertexBufferDescriptorInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet vertexBufferWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &vertexBufferDescriptorInfo);

    VkDescriptorBufferInfo triangleDataBufferDescriptorInfo{};
    triangleDataBufferDescriptorInfo.buffer = triangleDataBuffer.buffer;
    triangleDataBufferDescriptorInfo.offset = 0;
    triangleDataBufferDescriptorInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet triangleDataBufferWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5, &triangleDataBufferDescriptorInfo);

    VkDescriptorBufferInfo emissiveTriangleBufferDescriptorInfo{};
    emissiveTriangleBufferDescriptorInfo.buffer = emissiveTriangleBuffer.buffer;
    emissiveTriangleBufferDescriptorInfo.offset = 0;
    emissiveTriangleBufferDescriptorInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet emissiveTriangleBufferWrite = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &emissiveTriangleBufferDescriptorInfo);



    std::array<VkWriteDescriptorSet, 7> writes { writeAS, imageWrite, uniformBufferWrite, indexBufferWrite, vertexBufferWrite, triangleDataBufferWrite, emissiveTriangleBufferWrite };
    vkUpdateDescriptorSets(ctx.vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void RayTracer::cleanupFrameContext(FrameContext& frame) {
    auto& myFrame = frame.getExtFrame<RayTracerFrame>();
    buffertools::destroyBuffer(ctx, myFrame.cameraBuffer);
    vkDestroySampler(ctx.vkDevice, myFrame.blueNoiseSampler, nullptr);
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

Buffer RayTracer::createScratchBuffer(VkDeviceSize size) {
    Buffer scratchBuffer{};
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    buffertools::create_buffer_D(ctx, usage, size, &scratchBuffer);
    return scratchBuffer;
}

AccelerationStructure RayTracer::createAccelerationStructureBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    AccelerationStructure ret{};
    buffertools::create_buffer_D(ctx, usage, buildSizeInfo.accelerationStructureSize, &ret); 
    ret.deviceAddress = getBufferDeviceAddress(ret.buffer);                                                         
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
    auto indexBufferBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3);
    auto vertexBufferBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4);
    auto triangleDataBufferBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 5);
    auto emissiveTriangleBufferBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 6);
    std::vector<VkDescriptorSetLayoutBinding> bindings { ASLayoutBinding, resultImageLayoutBinding, uniformBufferBinding, indexBufferBinding, vertexBufferBinding, triangleDataBufferBinding, emissiveTriangleBufferBinding };

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
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
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
    rayTracingPipelineCI.maxPipelineRayRecursionDepth = 16;
    rayTracingPipelineCI.layout = pipelineLayout;
    vkCheck(vkCreateRayTracingPipelinesKHR(ctx.vkDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));

    for(const auto& stage : shaderStages) {
        vkDestroyShaderModule(ctx.vkDevice, stage.module, nullptr);
    }
}

void RayTracer::createBottomLevelAccelerationStructures() {
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;

    for (auto& mesh : info.meshes) {
        triangleDataOffsets.push_back(totalIndices/3);
        totalVertices += mesh->vertices.size();
        totalIndices += mesh->indices.size();
    }

    std::vector<Vertex> allVertices(totalVertices);
    std::vector<uint32_t> allIndices(totalIndices);
    std::vector<TriangleData> allTriangleData(totalIndices/3);
    std::vector<uint32_t> emissiveTriangles;
    emissiveTriangles.push_back(0);


    uint32_t vertexBufferOffset = 0;
    uint32_t indexBufferOffset = 0;
    uint32_t transformBufferOffset = 0;
    uint32_t modelIdx = 0;
    for (const auto& model : info.meshes) {
        memcpy(allVertices.data() + vertexBufferOffset, model->vertices.data(), model->vertices.size() * sizeof(Vertex));
        memcpy(allIndices.data() + indexBufferOffset, model->indices.data(), model->indices.size() * sizeof(uint32_t));
        assert(model->normals.size() == model->indices.size());
        for(uint32_t i=0; i<model->indices.size(); i+=3) {
            TriangleData triangleData{};
            triangleData.vertices[0] = model->vertices[model->indices[i+0]].v;
            triangleData.vertices[1] = model->vertices[model->indices[i+1]].v;
            triangleData.vertices[2] = model->vertices[model->indices[i+2]].v;

            if (modelIdx == 1) {
                triangleData.vertices[0].w = 5.0f;
                triangleData.vertices[1].w = 5.0f;
                triangleData.vertices[2].w = 5.0f;
                emissiveTriangles.push_back(indexBufferOffset/3 + i/3);
            }

            allTriangleData[indexBufferOffset/3 + i/3] = triangleData;
        }
        vertexBufferOffset += model->vertices.size();
        indexBufferOffset += model->indices.size();
        modelIdx++;
    }

    emissiveTriangles[0] = emissiveTriangles.size() - 1;

    float scale = 0.4f;
    VkTransformMatrixKHR transformMatrix = {
        scale, 0.0f, 0.0f, 0.0f,
        0.0f, scale, 0.0f, 0.0f,
        0.0f, 0.0f, scale, 0.0f,
    };

    std::vector<VkTransformMatrixKHR> allTransforms(info.meshes.size(), transformMatrix);

    const VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    buffertools::create_buffer_D_data(ctx, bufferUsage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, allVertices.size() * sizeof(Vertex), allVertices.data(), &vertexBuffer);
    buffertools::create_buffer_D_data(ctx, bufferUsage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, allIndices.size() * sizeof(uint32_t), allIndices.data(), &indexBuffer);
    buffertools::create_buffer_D_data(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, allTriangleData.size() * sizeof(TriangleData), allTriangleData.data(), &triangleDataBuffer);
    buffertools::create_buffer_D_data(ctx, bufferUsage, allTransforms.size() * sizeof(VkTransformMatrixKHR), allTransforms.data(), &transformBuffer);
    buffertools::create_buffer_D_data(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, emissiveTriangles.size() * sizeof(uint32_t), emissiveTriangles.data(), &emissiveTriangleBuffer);

    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(vertexBuffer.buffer);
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(indexBuffer.buffer);
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(transformBuffer.buffer);

    vertexBufferOffset = 0;
    indexBufferOffset = 0;
    for(const auto& model : info.meshes) {
        // The actual build
        VkAccelerationStructureGeometryTrianglesDataKHR triangleData {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT,
            .vertexData = vertexBufferDeviceAddress,
            .vertexStride = sizeof(Vertex),
            // TODO: why does this not seem to matter?
            .maxVertex = static_cast<uint32_t>(vertexBufferOffset + model->vertices.size()),
            .indexType = VK_INDEX_TYPE_UINT32,
            .indexData = indexBufferDeviceAddress,
            .transformData = transformBufferDeviceAddress,
        };
        

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.geometry.triangles = triangleData;

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        const uint32_t numTriangles = model->indices.size() / 3;
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo);

        bottomACs.push_back(createAccelerationStructureBuffer(accelerationStructureBuildSizesInfo));
        auto& bottomAC = bottomACs.back();

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = bottomAC.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCheck(vkCreateAccelerationStructureKHR(ctx.vkDevice, &accelerationStructureCreateInfo, nullptr, &bottomAC.AShandle));

        auto scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

        accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationStructureBuildGeometryInfo.dstAccelerationStructure = bottomAC.AShandle;
        accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer.buffer);

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
        accelerationStructureBuildRangeInfo.primitiveOffset = indexBufferOffset * sizeof(uint32_t);
        accelerationStructureBuildRangeInfo.firstVertex = vertexBufferOffset;
        accelerationStructureBuildRangeInfo.transformOffset = transformBufferOffset * sizeof(VkTransformMatrixKHR);

        auto rangeInfoPtr = &accelerationStructureBuildRangeInfo;

        auto cmdBuffer = ctx.singleTimeCommandBuffer();
        vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &accelerationStructureBuildGeometryInfo, &rangeInfoPtr);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = bottomAC.AShandle;
        bottomAC.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(ctx.vkDevice, &accelerationDeviceAddressInfo);
        ctx.endSingleTimeCommands(cmdBuffer);

        buffertools::destroyBuffer(ctx, scratchBuffer);
        vertexBufferOffset += model->vertices.size();
        indexBufferOffset += model->indices.size();
        transformBufferOffset += 1;
    }
}

void RayTracer::createTopLevelAccelerationStructure() {
    VkTransformMatrixKHR transformMatrix {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    for(uint32_t i=0; i<bottomACs.size(); i++) {
        const auto& bottomAC = bottomACs[i];
        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transformMatrix;
        instance.instanceCustomIndex = triangleDataOffsets[i];
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = bottomAC.deviceAddress;
        instances.push_back(instance);
    }

    Buffer instanceBuffer;
    const VkBufferUsageFlags usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    buffertools::create_buffer_D_data(ctx, usage, instances.size() * sizeof(VkAccelerationStructureInstanceKHR), instances.data(), &instanceBuffer);

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instanceBuffer.buffer);

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

    uint32_t primitiveCount = instances.size();

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(ctx.vkDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &buildSizesInfo);

    topAC = createAccelerationStructureBuffer(buildSizesInfo);

    VkAccelerationStructureCreateInfoKHR ASInfo{};
    ASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    ASInfo.buffer = topAC.buffer;
    ASInfo.size = buildSizesInfo.accelerationStructureSize;
    ASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCheck(vkCreateAccelerationStructureKHR(ctx.vkDevice, &ASInfo, nullptr, &topAC.AShandle));

    auto scratchBuffer = createScratchBuffer(buildSizesInfo.buildScratchSize);

    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure = topAC.AShandle;
    buildInfo.scratchData.deviceAddress = getBufferDeviceAddress(scratchBuffer.buffer);
 
    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = instances.size();
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr = &accelerationStructureBuildRangeInfo;

    auto cmdBuffer = ctx.singleTimeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(cmdBuffer, 1, &buildInfo, &buildRangeInfoPtr);
    ctx.endSingleTimeCommands(cmdBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = topAC.AShandle;
    topAC.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(ctx.vkDevice, &addressInfo);

    buffertools::destroyBuffer(ctx, scratchBuffer);
    buffertools::destroyBuffer(ctx, instanceBuffer);
}

void RayTracer::createShaderBindingTable() {
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAlligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const auto groupCount = static_cast<uint32_t>(shaderGroups.size());
    const uint32_t sbtSize = groupCount * handleSizeAlligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vkCheck(vkGetRayTracingShaderGroupHandlesKHR(ctx.vkDevice, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    buffertools::create_buffer_D_data(ctx, bufferUsageFlags, handleSize, shaderHandleStorage.data() + handleSizeAlligned * 0, &raygenShaderBindingTable);
    buffertools::create_buffer_D_data(ctx, bufferUsageFlags, handleSize, shaderHandleStorage.data() + handleSizeAlligned * 1, &missShaderBindingTable);
    buffertools::create_buffer_D_data(ctx, bufferUsageFlags, handleSize, shaderHandleStorage.data() + handleSizeAlligned * 2, &hitShaderBindingTable);
}

void RayTracer::destroyAccelerationStructure(AccelerationStructure& structure) const {
    buffertools::destroyBuffer(ctx, structure);
    vkDestroyAccelerationStructureKHR(ctx.vkDevice, structure.AShandle, nullptr);
}


void RayTracer::render(FrameContext& frame, const Camera& camera) {
    const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);

    auto& wFrame = frame.getExtFrame<WindowFrame>();
    const float aspectRatio = (float)wFrame.width / (float)wFrame.height;
    glm::mat4 projectionMatrix = glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f);


    RayTracerCamera cameraInfo {
        .viewInverse = glm::inverse(camera.getViewMatrix()),
        .projInverse = glm::inverse(projectionMatrix),
        .viewDir = glm::vec4(camera.getViewDir(), 0),
    };

    cameraInfo.setTime(static_cast<float>(glfwGetTime()));
    cameraInfo.setTick(tick++);
    cameraInfo.setShouldReset(shouldReset);

    auto& myFrame = frame.getExtFrame<RayTracerFrame>();
    void* data;
    vkCheck(vmaMapMemory(ctx.vmaAllocator, myFrame.cameraBuffer.memory, &data));
    memcpy(data, &cameraInfo, sizeof(RayTracerCamera));
    vmaUnmapMemory(ctx.vmaAllocator, myFrame.cameraBuffer.memory);
    vmaFlushAllocation(ctx.vmaAllocator, myFrame.cameraBuffer.memory, 0, sizeof(RayTracerCamera));


    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(raygenShaderBindingTable.buffer);
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(missShaderBindingTable.buffer);
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(hitShaderBindingTable.buffer);
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &frame.getExtFrame<RayTracerFrame>().descriptorSet, 0, 0);
    vkCmdTraceRaysKHR(frame.cmdBuffer, &raygenShaderSbtEntry, &missShaderSbtEntry, &hitShaderSbtEntry, &callableShaderSbtEntry, wFrame.width, wFrame.height,1);

    shouldReset = false;
}

}
