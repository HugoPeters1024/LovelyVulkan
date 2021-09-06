#include <liftedvulkan.h>

const lv::ImageID COMPUTE_IMAGE_1 = lv::ImageID(0);
const lv::ImageID COMPUTE_IMAGE_2 = lv::ImageID(1);


///
// PROBLEM AT HAND:
// number of swapchain images is hardware defined and >1.
// every frame we take the next swapchain image.
// this breaks to one-to-one relation between swapchain images and frames
// this breaks the callback selectors


int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;
    lv::AppContext ctx(info);

    lv::ImageStoreInfo imageStoreInfo;
    imageStoreInfo.defineImage(COMPUTE_IMAGE_1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_GENERAL);
    imageStoreInfo.defineImage(COMPUTE_IMAGE_2, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_GENERAL);
    auto imageStore = ctx.registerExtension<lv::ImageStore>(imageStoreInfo);

    lv::ComputeShaderInfo compInfo{};
    compInfo.addImageBinding(0, 0, [](lv::FrameContext& frame) { return &frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE_1).view; });
    compInfo.addImageBinding(0, 1, [](lv::FrameContext& frame) { return &frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE_2).view; });
    compInfo.setPushConstantType<float>();
    auto computeShader = ctx.registerExtension<lv::ComputeShader>("app/shaders_bin/test.comp.spv", compInfo);

    lv::RasterizerInfo rastInfo("app/shaders_bin/quad.vert.spv", "app/shaders_bin/quad.frag.spv");
    rastInfo.defineAttachment(0, [](lv::FrameContext& frame) { return frame.swapchain.vkView; });
    rastInfo.defineTexture(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE_2).view; });
    auto rasterizer = ctx.registerExtension<lv::Rasterizer>(rastInfo);

    lv::WindowInfo windowInfo;
    windowInfo.width = 640;
    windowInfo.height = 480;
    windowInfo.windowName = "Lovely Vulkan";
    lv::Window window(ctx, windowInfo);
    while(!window.shouldClose()) {
        window.nextFrame([computeShader, rasterizer](lv::FrameContext& frame) {
            auto compFrame = frame.getExtFrame<lv::ComputeFrame>();
            auto imgStore = frame.getExtFrame<lv::ImageStoreFrame>();
            auto rastFrame = frame.getExtFrame<lv::RasterizerFrame>();

            vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipelineLayout, 0, 1, &compFrame.descriptorSets[0], 0, nullptr);
            vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipeline);
            float time = static_cast<float>(glfwGetTime());
            vkCmdPushConstants(frame.commandBuffer, computeShader->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &time);
            vkCmdDispatch(frame.commandBuffer, frame.swapchain.width/16, frame.swapchain.height/16, 1);
            
            auto barrier = vks::initializers::imageMemoryBarrier(
                    frame.swapchain.vkImage,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

            // Prepare the image to be sampled when rendering to the screen
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.get(COMPUTE_IMAGE_2).image,
                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

            rasterizer->startPass(frame);
            vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);
            rasterizer->endPass(frame);

            // Prepare the image to be sampled when rendering to the screen
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.get(COMPUTE_IMAGE_2).image,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
        });
    }

    logger::info("Goodbye!");
    return 0;
}
