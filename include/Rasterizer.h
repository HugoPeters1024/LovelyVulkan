#pragma once
#include "precomp.h"
#include "AppContext.h"
#include "AppExt.h"
#include "Window.h"

namespace lv {

struct RasterizerAttachment {
    uint32_t binding;
    FrameSelector<VkImageView> imageSelector;
};

struct RasterizerTexture {
    uint32_t binding;
    FrameSelector<VkImageView> imageSelector;
};

struct RasterizerInfo {
    RasterizerInfo(std::string vertShaderPath, std::string fragShaderPath)
        : vertShaderPath(vertShaderPath), fragShaderPath(fragShaderPath) {}

    void defineAttachment(uint32_t slot, FrameSelector<VkImageView> imageSelector) {
        attachments.push_back(RasterizerAttachment {slot, imageSelector});
    }

    void defineTexture(uint32_t slot, FrameSelector<VkImageView> imageSelector) {
        textures.push_back(RasterizerTexture {slot, imageSelector});
    }


    std::string vertShaderPath;
    std::string fragShaderPath;
    std::vector<RasterizerAttachment> attachments;
    std::vector<RasterizerTexture> textures;
};

struct RasterizerFrame : public FrameExt {
    VkDescriptorSet descriptorSet;
    VkFramebuffer framebuffer;
    VkSampler sampler;
};

class Rasterizer : public AppExt {
public:
    Rasterizer(AppContext& ctx, RasterizerInfo info);
    ~Rasterizer();

    void embellishFrameContext(FrameContext& frame) override;
    void cleanupFrameContext(FrameContext& frame) override;

    void startPass(FrameContext& frame) const;
    void endPass(FrameContext& frame) const;

    VkRenderPass getRenderPass() const { return renderPass; }

private:
    void createRenderPass();
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();



//------------- FIELDS -------------------

private:
    RasterizerInfo info;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptorSetLayout;
};

}
