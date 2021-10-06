#include "Overlay.h"

namespace lv {

Overlay::Overlay(AppContext& ctx, OverlayInfo info) : AppExt(ctx), info(info) {
    createDescriptorPool();
    initImgui();
}

Overlay::~Overlay() {
    vkDestroyDescriptorPool(ctx.vkDevice, imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void Overlay::createDescriptorPool() {
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
            {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    vkCheck(vkCreateDescriptorPool(ctx.vkDevice, &pool_info, nullptr, &imguiPool));
}

void Overlay::initImgui() {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(info.glfwWindow, true);

     ImGui_ImplVulkan_InitInfo initInfo {
         .Instance = ctx.vkInstance,
         .PhysicalDevice = ctx.vkPhysicalDevice,
         .Device = ctx.vkDevice,
         .Queue = ctx.queues.graphics,
         .DescriptorPool = imguiPool,
         .MinImageCount = info.frameManager->getNrFrames(),
         .ImageCount= info.frameManager->getNrFrames(),
         .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
     };
   
     ImGui_ImplVulkan_Init(&initInfo, info.renderPass);
   
     auto initCmd = ctx.singleTimeCommandBuffer();
     ImGui_ImplVulkan_CreateFontsTexture(initCmd);
     ctx.endSingleTimeCommands(initCmd);
   
     ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Overlay::render(FrameContext& frame, float energy) {
    ImGui_ImplVulkanH_Frame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("test")) {
        ImGui::Text("Energy %f", energy);
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.cmdBuffer);
}

}
