#pragma once
#include <utility>

#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "Window.h"

namespace lv {

class ComputeShader;

template<>
struct app_extensions<ComputeShader> {
    void operator()(AppContextInfo& info) const { 
        info.deviceExtensions.insert(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
    }
};

enum class ResourceType { Image, Buffer };
struct ComputeShaderBindingInfo {
    uint32_t binding;
    ResourceType type;
    FrameSelector<VkImageView> viewSelector;
    FrameSelector<VkBuffer> bufferSelector;
};

struct ComputeShaderInfo {
    std::unordered_map<uint32_t, ComputeShaderBindingInfo> bindingSet;
    size_t pushConstantSize = 0;
    std::type_index pushConstantType = typeid(void);

    inline void addImageBinding(uint32_t binding, FrameSelector<VkImageView> selector) { 
        if (bindingSet.find(binding) != bindingSet.end()) {
            assert(false && "Binding already populated");
        }
        
        bindingSet[binding] = ComputeShaderBindingInfo {
            .binding = binding,
            .type = ResourceType::Image,
            .viewSelector = std::move(selector),
        };
    }

    inline void addBufferBinding(uint32_t binding, FrameSelector<VkBuffer> selector) {
        assert(bindingSet.find(binding) == bindingSet.end() && "Binding already populated");
        bindingSet[binding] = ComputeShaderBindingInfo {
            .binding = binding,
            .type = ResourceType::Buffer,
            .bufferSelector = std::move(selector),
        };
    }
    

    template<typename T>
    inline void setPushConstantType() { 
        pushConstantSize = sizeof(T); 
        pushConstantType = typeid(T);
    }
};

struct ComputeFrame : public FrameExt {
    VkDescriptorSet descriptorSet;
};

class ComputeShader : public AppExt {
public:
    ComputeShader(AppContext& ctx, const char* filePath, ComputeShaderInfo info);
    ~ComputeShader() override;

    VkShaderModule shaderModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptorSetLayout;

    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;

private:
    const char* filePath;
    ComputeShaderInfo info;
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
};

}
