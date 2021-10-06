#include "ComputeShader.h"

namespace lv {

ComputeShader::ComputeShader(AppContext& ctx, const char* filePath, ComputeShaderInfo info) 
    : AppExt(ctx), filePath(filePath), info(info) {
    logger::debug("Core of comp shader being build");
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
}

ComputeShader::~ComputeShader() {
    vkDestroyPipeline(ctx.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.vkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.vkDevice, descriptorSetLayout, nullptr);
}

void ComputeShader::createDescriptorSetLayout() {
    logger::debug("Creating descriptor set");
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for(auto& pair : info.bindingSet) {
        auto& binding = pair.second;

        VkDescriptorType descriptorType;
        switch(binding.type) {
            case ResourceType::Image: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
            case ResourceType::Buffer: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
        }

        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding = binding.binding,
            .descriptorType = descriptorType,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        });
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCheck(vkCreateDescriptorSetLayout(ctx.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void ComputeShader::createPipelineLayout() {
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = static_cast<uint32_t>(info.pushConstantSize),
    };

    if (pushConstantRange.size == 0) {
        logger::debug("PushConstant range for compute shader unused");
    }

    VkPipelineLayoutCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = pushConstantRange.size > 0 ? 1u : 0u,
        .pPushConstantRanges = pushConstantRange.size > 0 ? &pushConstantRange : nullptr,
    };

    vkCheck(vkCreatePipelineLayout(ctx.vkDevice, &createInfo, nullptr, &pipelineLayout));
}

void ComputeShader::createPipeline() {
    auto module = ctx.createShaderModule(filePath);
    auto pipelineInfo = vks::initializers::computePipelineCreateInfo(pipelineLayout);
    pipelineInfo.stage = vks::initializers::pipelineShaderStageCreateInfo(module, VK_SHADER_STAGE_COMPUTE_BIT);
    vkCheck(vkCreateComputePipelines(ctx.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline));
    vkDestroyShaderModule(ctx.vkDevice, module, nullptr);
}

void ComputeShader::embellishFrameContext(FrameContext& frame) {
    auto& ret = frame.registerExtFrame<ComputeFrame>();

    std::vector<VkDescriptorSetLayout> layouts { descriptorSetLayout };
    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(ctx.vkDescriptorPool, layouts);
    vkCheck(vkAllocateDescriptorSets(ctx.vkDevice, &allocInfo, &ret.descriptorSet));

    for(const auto& pair : info.bindingSet) {
        auto& binding = pair.second;
        if (binding.type == ResourceType::Image) {
            // TODO: make sure that the imageInfos are sorted by binding
            VkImageView imageView = binding.viewSelector(frame);
            auto imageInfo = vks::initializers::descriptorImageInfo(nullptr, imageView, VK_IMAGE_LAYOUT_GENERAL);
            auto writeInfo = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, binding.binding, &imageInfo);
            vkUpdateDescriptorSets(frame.ctx.vkDevice, 1, &writeInfo, 0, nullptr);
        } else {
            VkBuffer buffer = binding.bufferSelector(frame);
            auto bufferInfo = vks::initializers::descriptorBufferInfo(buffer);
            auto writeInfo = vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding.binding, &bufferInfo);
            vkUpdateDescriptorSets(frame.ctx.vkDevice, 1, &writeInfo, 0, nullptr);
        }
    }
}

void ComputeShader::cleanupFrameContext(FrameContext& frame) {
}

}
