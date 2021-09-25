#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "BufferTools.h"

namespace lv {

class RayTracer;

struct AccelerationStructure : public Buffer {
    VkAccelerationStructureKHR AShandle;
    uint64_t deviceAddress = 0;
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
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        }; 
    }
};

struct RayTracerCamera {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    glm::vec3 pad0;
    float time;
    glm::vec3 pad1;
    uint tick;
    glm::vec3 pad2;
    bool shouldReset;
};

struct RayTracerFrame {
    VkDescriptorSet descriptorSet;
    Buffer cameraBuffer;
};


struct RayTracerInfo {
    struct Vertex { float pos[4]; float emission[4]; };
    std::vector<Vertex> vertexData;
    std::vector<uint32_t> indexData;
};

class RayTracer : public AppExt<RayTracerFrame> {
public:
    RayTracer(AppContext& ctx, RayTracerInfo info);
    ~RayTracer();


    RayTracerFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(RayTracerFrame* frame) override;
    void render(FrameContext& frame, glm::mat4 viewMatrix);

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
    void createBottomLevelAccelerationStructure();
    void createTopLevelAccelerationStructure();
    void createShaderBindingTable();

    void destroyAccelerationStructure(AccelerationStructure& structure) const;
    uint64_t getBufferDeviceAddress(VkBuffer buffer) const;

    RayTracerInfo info;

    Buffer vertexBuffer; 
    Buffer indexBuffer;
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

    AccelerationStructure bottomAC;
    AccelerationStructure topAC;

    bool shouldReset = false;
};


};

