#pragma once
#include "precomp.h"

namespace lv {

struct AttachmentView {
    VkFormat format;
    uint32_t binding;
    VkImageLayout finalLayout;
    std::vector<VkImage>* images;
    std::vector<VkImageView>* imageViews;
};

struct ColorAttachment {
    VkFormat format;
    uint32_t binding;
    VkImageLayout finalLayout;
    std::vector<VkImage> images;
    std::vector<VmaAllocation> imagesMemory;
    std::vector<VkImageView> imageViews;

    inline AttachmentView createView() {
        return AttachmentView {
            .format = format,
            .binding = binding,
            .finalLayout = finalLayout,
            .images = &images,
            .imageViews = &imageViews,
        };
    }
};

struct ComputeShaderImage {
    uint32_t binding;

    VkDescriptorSetLayoutBinding getLayoutBinding() const {
        return VkDescriptorSetLayoutBinding {
            .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        };
    }
};

}