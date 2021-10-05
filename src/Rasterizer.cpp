#include "Rasterizer.h"

namespace lv {

Rasterizer::Rasterizer(AppContext& ctx, RasterizerInfo info) 
    : AppExt(ctx), info(info) {
    createRenderPass();
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
}

Rasterizer::~Rasterizer() {
    vkDestroyPipeline(ctx.vkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.vkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.vkDevice, descriptorSetLayout, nullptr);
    vkDestroyRenderPass(ctx.vkDevice, renderPass, nullptr);
}

void Rasterizer::embellishFrameContext(FrameContext& frame) {
    auto& ret = frame.registerExtFrame<RasterizerFrame>();
    auto allocInfo = vks::initializers::descriptorSetAllocateInfo(ctx.vkDescriptorPool, &descriptorSetLayout, 1);
    vkCheck(vkAllocateDescriptorSets(frame.ctx.vkDevice, &allocInfo, &ret.descriptorSet))

    auto samplerInfo = vks::initializers::samplerCreateInfo(1.0f);
    vkCheck(vkCreateSampler(ctx.vkDevice, &samplerInfo, nullptr, &ret.sampler));

    std::vector<VkDescriptorImageInfo> descriptorImages;
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    for(const auto& texture : info.textures) {
        descriptorImages.push_back(vks::initializers::descriptorImageInfo(ret.sampler, texture.imageSelector(frame), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        auto& texInfo = descriptorImages.back();
        descriptorWrites.push_back(vks::initializers::writeDescriptorSet(ret.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture.binding, &texInfo));
    }

    vkUpdateDescriptorSets(ctx.vkDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    
    std::vector<VkImageView> attachments;
    for(const auto& attachmentInfo : info.attachments) {
        attachments.push_back(attachmentInfo.imageSelector(frame));
    }

    auto& wFrame = frame.getExtFrame<WindowFrame>();
    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = wFrame.width, 
        .height = wFrame.height,
        .layers = 1,
    };

    vkCheck(vkCreateFramebuffer(ctx.vkDevice, &framebufferInfo, nullptr, &ret.framebuffer));

}

void Rasterizer::cleanupFrameContext(FrameContext& frame) {
    auto& myFrame = frame.getExtFrame<RasterizerFrame>();
    vkDestroyFramebuffer(ctx.vkDevice, myFrame.framebuffer, nullptr);
    vkDestroySampler(ctx.vkDevice, myFrame.sampler, nullptr);
}

void Rasterizer::startPass(FrameContext& frame) const {
    std::vector<VkClearValue> clearValues(info.attachments.size(), VkClearValue { 0.0f, 0.0f, 0.0f, 0.0f });

    auto& myFrame = frame.getExtFrame<RasterizerFrame>();
    auto& wFrame = frame.getExtFrame<WindowFrame>();

    VkRenderPassBeginInfo renderPassInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = myFrame.framebuffer,
        .renderArea = {
                .offset = {0,0},
                .extent = {wFrame.width, wFrame.height},
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
    VkViewport viewport {
            .x = 0.0f,
            .y = static_cast<float>(wFrame.height),
            .width = static_cast<float>(wFrame.width),
            .height = -static_cast<float>(wFrame.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    VkRect2D scissor{{0,0}, {wFrame.width, wFrame.height}};

    vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &myFrame.descriptorSet, 0, nullptr);
}

void Rasterizer::endPass(FrameContext& frame) const {
    vkCmdEndRenderPass(frame.cmdBuffer);
}

void Rasterizer::createRenderPass() {
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> attachmentRefs;

    for(const auto& attachmentInfo : info.attachments) {
        attachments.push_back(VkAttachmentDescription {
            // TODO: not hard code this, probably have AppContext already decide
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        });

        attachmentRefs.push_back(VkAttachmentReference {
            .attachment = attachmentInfo.binding,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size()),
        .pColorAttachments = attachmentRefs.data(),
        .pDepthStencilAttachment = nullptr,
    };

    VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    vkCheck(vkCreateRenderPass(ctx.vkDevice, &createInfo, nullptr, &renderPass));
}

void Rasterizer::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for(const auto& texInfo : info.textures) {
        bindings.push_back(VkDescriptorSetLayoutBinding {
            .binding = texInfo.binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        });
    }
    auto layoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(bindings);
    vkCheck(vkCreateDescriptorSetLayout(ctx.vkDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void Rasterizer::createPipelineLayout() {
    auto layoutInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout);
    vkCheck(vkCreatePipelineLayout(ctx.vkDevice, &layoutInfo, nullptr, &pipelineLayout));
}

void Rasterizer::createPipeline() {
    auto vertShader = ctx.createShaderModule(info.vertShaderPath.c_str());
    auto fragShader = ctx.createShaderModule(info.fragShaderPath.c_str());

    auto inputAssembly = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    auto rasterization = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    auto blendAttachment = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE);
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    auto colorBlend = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachment);
    auto depthStencil = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    auto viewport = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
    auto multisample = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    auto dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        vks::initializers::pipelineShaderStageCreateInfo(vertShader, VK_SHADER_STAGE_VERTEX_BIT),
        vks::initializers::pipelineShaderStageCreateInfo(fragShader, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    auto vertexInput = vks::initializers::pipelineVertexInputStateCreateInfo();

    VkGraphicsPipelineCreateInfo pipelineInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlend,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0
    };

    vkCheck(vkCreateGraphicsPipelines(ctx.vkDevice, nullptr, 1, &pipelineInfo, nullptr, &pipeline));

    vkDestroyShaderModule(ctx.vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(ctx.vkDevice, fragShader, nullptr);
}


}
