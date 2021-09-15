#pragma once
#include <utility>

#include "precomp.h"
#include "AppContext.h"

namespace lv {

typedef uint32_t DescriptorId;
enum class ResourceType { Image, Buffer };
struct ComputeShaderBindingInfo {
    uint32_t binding;
    ResourceType type;
    DescriptorId descriptorId;
    std::function<void*(FrameContext&)> selector;
};

struct ComputeShaderInfo {
    std::unordered_map<uint32_t, ResourceType> bindingSet;
    std::unordered_map<DescriptorId, std::vector<ComputeShaderBindingInfo>> descriptorSelectorsTable;
    size_t pushConstantSize = 0;
    std::type_index pushConstantType = typeid(void);

    inline void addImageBinding(DescriptorId descriptorId, uint32_t binding, std::function<VkImageView*(FrameContext&)> selector) { 
        if (bindingSet.find(binding) == bindingSet.end()) {
            bindingSet[binding] = ResourceType::Image;
        }
        
        descriptorSelectorsTable[descriptorId].push_back(ComputeShaderBindingInfo {
            .binding = binding,
            .type = ResourceType::Image,
            .descriptorId = descriptorId,
            .selector = std::move(selector),
        });
    }

    template<typename T>
    inline void setPushConstantType() { 
        pushConstantSize = sizeof(T); 
        pushConstantType = typeid(T);
    }
};

struct ComputeFrame {
    std::vector<VkDescriptorSet> descriptorSets;
};

class ComputeShader : public AppExt<ComputeFrame> {
public:
    ComputeShader(AppContext& ctx, const char* filePath, ComputeShaderInfo info);
    ~ComputeShader() override;

    VkShaderModule shaderModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptorSetLayout;


    ComputeFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(ComputeFrame* frame) override {}
private:
    const char* filePath;
    ComputeShaderInfo info;
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
};

}
