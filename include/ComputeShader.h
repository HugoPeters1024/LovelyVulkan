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
private:
    ComputeShaderInfo info;
    void createDescriptorSetLayout(AppContext& ctx);
    void createPipelineLayout(AppContext &ctx);
    void createPipeline(AppContext &ctx, const char *filePath);


public:
    VkShaderModule shaderModule;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriporSetLayout;
    const char* filePath;

    ComputeShader(const char* filePath, ComputeShaderInfo info) 
        : filePath(filePath), info(info) { }


    void buildCore(AppContext& ctx) override;
    void destroyCore(AppContext& ctx) override;
    ComputeFrame* buildFrame(FrameContext& frame) override;
    void destroyFrame(AppContext& ctx, ComputeFrame*) override {}
};

}
