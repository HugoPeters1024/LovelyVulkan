#pragma once
#include "precomp.h"
#include "AppContext.h"

namespace lv {

struct ComputeFrame {
    VkDescriptorSet descriptorSet;
};

enum class ResourceType { Image, Buffer };
struct ComputeShaderBindingInfo {
    ResourceType type;
    std::function<void*(FrameContext&)> selector;
};

struct ComputeShaderInfo {
    std::unordered_map<uint32_t, ComputeShaderBindingInfo> bindingSet;

    inline void addImageBinding(uint32_t binding, std::function<VkImageView*(FrameContext&)> selector) { 
        bindingSet[binding] = ComputeShaderBindingInfo {
            .type = ResourceType::Image,
            .selector = selector,
        };
    }
};

class ComputeShader : public AppExt<ComputeFrame> {
public:
    ComputeShader(AppContext& ctx, const char* filePath, ComputeShaderInfo info);
    ~ComputeShader();

    VkShaderModule shaderModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriporSetLayout;


    ComputeFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(AppContext& ctx, ComputeFrame*) override {}
private:
    const char* filePath;
    ComputeShaderInfo info;
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
};

}
