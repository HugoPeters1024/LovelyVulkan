#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "BufferTools.h"
#include "ImageTools.h"
#include "ResourceStore.h"
#include "Camera.h"
#include "Mesh.h"
#include "Window.h"

namespace lv {

class RayTracer;

struct AccelerationStructure : public Buffer {
    VkAccelerationStructureKHR AShandle;
    uint64_t deviceAddress = 0;
};

template<>
struct app_extensions<RayTracer> {
    void operator()(AppContextInfo& info) const { 
        logger::debug("Enabling Vulkan RTX");
        info.deviceExtensions.insert(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        info.deviceExtensions.insert(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    }
};

struct RayTracerCamera {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec4 viewDir;
    glm::vec4 properties0;


    inline void setTime(float time) { properties0[0] = time; }
    inline void setTick(uint32_t tick) { properties0[1] = reinterpret_cast<float&>(tick); }
    inline void setShouldReset(bool shouldReset) { properties0.z = shouldReset ? 1.0f : 0.0f; }
};

struct TriangleData {
    glm::vec4 normals[3];
};


struct RayTracerFrame : public FrameExt {
    VkDescriptorSet descriptorSet;
    Buffer cameraBuffer;
    VkSampler blueNoiseSampler;
};


struct RayTracerInfo {
    std::vector<const Mesh*> meshes;
};

class RayTracer : public AppExt {
public:
    RayTracer(AppContext& ctx, RayTracerInfo info);
    ~RayTracer();

    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;

    void render(FrameContext& frame, const Camera& camera);

    inline void resetAccumulator() { shouldReset = true; }

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
    Buffer createScratchBuffer(VkDeviceSize size);
    AccelerationStructure createAccelerationStructureBuffer(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

    void getFeatures();
    void createRayTracingPipeline();
    void createBottomLevelAccelerationStructures();
    void createTopLevelAccelerationStructure();
    void createShaderBindingTable();

    void destroyAccelerationStructure(AccelerationStructure& structure) const;
    uint64_t getBufferDeviceAddress(VkBuffer buffer) const;

    RayTracerInfo info;

    Buffer vertexBuffer; 
    Buffer indexBuffer;
    Buffer triangleDataBuffer;
    Buffer transformBuffer;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    Buffer raygenShaderBindingTable;
    Buffer missShaderBindingTable;
    Buffer hitShaderBindingTable;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};


    std::vector<AccelerationStructure> bottomACs;
    std::vector<uint32_t> triangleDataOffsets;
    AccelerationStructure topAC;

    Image blueNoise;

    bool shouldReset = false;
    uint32_t tick = 0;
};


};

