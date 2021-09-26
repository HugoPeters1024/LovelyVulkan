#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "BufferTools.h"
#include "ImageStore.h"
#include "Camera.h"

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
    alignas(16) glm::mat4 viewInverse;
    alignas(16) glm::mat4 projInverse;
    alignas(16) glm::vec4 viewDir;
    alignas(16) glm::vec4 properties0;

    inline void setTime(float time) { properties0[0] = time; }
    inline void setTick(uint32_t tick) { properties0[1] = reinterpret_cast<float&>(tick); }
    inline void setShouldReset(bool shouldReset) { properties0.z = shouldReset ? 1.0f : 0.0f; }
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

