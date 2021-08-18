#include "ComputeShader.h"

namespace lv {

void ComputeShaderBindingSet::bindImage(uint32_t binding, VkImageView imageView) {
    assert(device && "create binding set only from an existing shader");
    for(const auto& descriptorSet : descriptorSets) {
        auto imageInfo = vks::initializers::descriptorImageInfo(nullptr, imageView, VK_IMAGE_LAYOUT_GENERAL);
        auto writeInfo = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, binding, &imageInfo);
        vkUpdateDescriptorSets(device->getVkDevice(), 1, &writeInfo, 0, nullptr);
    }
}

ComputeShader::ComputeShader(Device &device, const ComputeShaderCore &core) : device(device), core(core) {
    buildDescriptorSetLayout();
    buildPipelineLayout();
    buildPipeline();
}

ComputeShader::~ComputeShader() {
    vkDestroyPipeline(device.getVkDevice(), pipeline, nullptr);
    vkDestroyPipelineLayout(device.getVkDevice(), pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device.getVkDevice(), descriptorSetLayout, nullptr);
}

void ComputeShader::buildDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for(const auto& imageBinding : core.images) {
        bindings.push_back(imageBinding.getLayoutBinding());
    }

    auto layoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(bindings);
    vkCheck(vkCreateDescriptorSetLayout(device.getVkDevice(), &layoutInfo, nullptr, &descriptorSetLayout));
}

void ComputeShader::buildPipelineLayout() {
    auto pipelineInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout);
    vkCheck(vkCreatePipelineLayout(device.getVkDevice(), &pipelineInfo, nullptr, &pipelineLayout));
}

void ComputeShader::buildPipeline() {
    auto module = device.createShaderModule(core.filename);
    auto pipelineInfo = vks::initializers::computePipelineCreateInfo(pipelineLayout);
    pipelineInfo.stage = vks::initializers::pipelineShaderStageCreateInfo(module, VK_SHADER_STAGE_COMPUTE_BIT);
    vkCheck(vkCreateComputePipelines(device.getVkDevice(), nullptr, 1, &pipelineInfo, nullptr, &pipeline));
    vkDestroyShaderModule(device.getVkDevice(), module, nullptr);
}

void ComputeShader::createBindingSet(ComputeShaderBindingSet *dest) {
    dest->device = &device;
    std::vector<VkDescriptorSetLayout> layouts(core.duplication, descriptorSetLayout);
    dest->descriptorSets.resize(core.duplication);
    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(device.getVkDescriptorPool(), layouts.data(), layouts.size());
    vkCheck(vkAllocateDescriptorSets(device.getVkDevice(), &allocInfo, dest->descriptorSets.data()));
}


}