#pragma once
#include "precomp.h"
#include "Device.h"
#include "Structs.hpp"

namespace lv {

class ComputeShaderBindingSet {
public:
    Device* device;
    std::vector<VkDescriptorSet> descriptorSets;
    void bindImage(uint32_t binding, VkImageView image);
};

class ComputeShaderCore {
public:
    std::string filename;
    std::vector<ComputeShaderImage> images;

    uint32_t duplication;
};

class ComputeShader {
private:
    Device& device;
    const ComputeShaderCore& core;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void buildDescriptorSetLayout();
    void buildPipelineLayout();
    void buildPipeline();

public:
    ComputeShader(Device& device, const ComputeShaderCore& core);
    ~ComputeShader();

    void createBindingSet(ComputeShaderBindingSet* dest);
};

}