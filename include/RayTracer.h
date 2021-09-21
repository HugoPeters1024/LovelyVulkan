#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"

namespace lv {

class RayTracer;

struct RayTracingScratchBuffer {
    uint64_t deviceAddress = 0;
    VkBuffer handle = VK_NULL_HANDLE;
    VmaAllocation memory = VK_NULL_HANDLE;
};

struct AccelerationStructure {
    VkAccelerationStructureKHR handle;
    uint64_t deviceAddress = 0;
    VmaAllocation memory;
    VkBuffer buffer;
};

template<>
struct app_extensions<RayTracer> {
    std::vector<const char*> operator()() const { 
        return {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        }; 
    }
};

struct RayTracerFrame {
};

struct RayTracerInfo {
};

class RayTracer : public AppExt<RayTracerFrame> {
public:
    RayTracer(AppContext& ctx, RayTracerInfo info);
    ~RayTracer();


    RayTracerFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(RayTracerFrame* frame) override;

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

private:
    void loadFunctions();
    RayTracingScratchBuffer createScratchBuffer(VkDeviceSize size);
    void destroyScratchBuffer(RayTracingScratchBuffer& scratchBuffer);
    AccelerationStructure createAccelerationStructureBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

    void createBottomLevelAccelerationStructure();
    void createTopLevelAccelerationStructure();

    uint64_t getBufferDeviceAddress(VkBuffer buffer);

    RayTracerInfo info;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferMemory;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferMemory;
    VkBuffer transformBuffer;
    VmaAllocation transformBufferMemory;

    AccelerationStructure bottomAC;
    AccelerationStructure topAC;
};


};

