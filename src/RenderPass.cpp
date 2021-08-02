#include "RenderPass.h"

namespace lv {

RenderPassCore::RenderPassCore(Device &device) : device(device) {

}

RenderPassCore::~RenderPassCore() {
    for(auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(device.getVkDevice(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(device.getVkDevice(), renderPass, nullptr);

    for(auto& attachment : attachments) {
        for(int i=0; i<duplication; i++) {
            vmaDestroyImage(device.getVmaAllocator(), attachment.images[i], attachment.imagesMemory[i]);
            vkDestroyImageView(device.getVkDevice(), attachment.imageViews[i], nullptr);
        }
    }
}

void RenderPassCore::build() {
    finalizeState();
    createAttachments();
    createRenderPass();
    createFramebuffers();
}

void RenderPassCore::finalizeState() {
    if (parent) {
        duplication = parent->getNrImages();
        width = parent->getWidth();
        height = parent->getHeight();
    }
}

void RenderPassCore::createAttachments() {
    for(auto& attachment : attachments) {
        attachment.images.resize(duplication);
        attachment.imagesMemory.resize(duplication);
        attachment.imageViews.resize(duplication);
        auto imageInfo = vks::initializers::imageCreateInfo(width, height, attachment.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        for(uint32_t i=0; i<duplication; i++) {
            device.createDeviceImage(imageInfo, &attachment.images[i], &attachment.imagesMemory[i]);
            auto viewInfo = vks::initializers::imageViewCreateInfo(attachment.images[i], attachment.format, VK_IMAGE_ASPECT_COLOR_BIT);
            vkCheck(vkCreateImageView(device.getVkDevice(), &viewInfo, nullptr, &attachment.imageViews[i]));
        }
    }
}

void RenderPassCore::createRenderPass() {
    VkAttachmentDescription attachmentDescriptions[attachments.size()];
    VkAttachmentReference attachmentReferences[attachments.size()];

    for(int i=0; i<attachments.size(); i++) {
        attachmentDescriptions[i] = vks::initializers::attachmentDescription(attachments[i].format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        attachmentReferences[i] = vks::initializers::attachmentReference(attachments[i].binding, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    VkSubpassDescription subpassInfo {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(attachments.size()),
        .pColorAttachments = attachmentReferences,
        .pDepthStencilAttachment = nullptr,
    };

    VkRenderPassCreateInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachmentDescriptions,
        .subpassCount = 1,
        .pSubpasses = &subpassInfo,
        .dependencyCount = 0,
    };

    vkCheck(vkCreateRenderPass(device.getVkDevice(), &renderPassInfo, nullptr, &renderPass))
}

void RenderPassCore::createFramebuffers() {
    framebuffers.resize(duplication);
    for(uint32_t i=0; i<duplication; i++) {
        VkImageView attachmentViews[attachments.size()];
        for(uint32_t j=0; j<attachments.size(); j++) {
            attachmentViews[j] = attachments[j].imageViews[i];
        }

        VkFramebufferCreateInfo framebufferInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachmentViews,
            .width = width,
            .height = height,
            .layers = 1,
        };

        vkCheck(vkCreateFramebuffer(device.getVkDevice(), &framebufferInfo, nullptr, &framebuffers[i]));
    }
}

}
