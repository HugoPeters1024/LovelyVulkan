#include <liftedvulkan.h>

enum Images {
    COMPUTE_IMAGE,
};


int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;
    lv::AppContext ctx(info);

    lv::ImageStoreInfo imageStoreInfo;
    imageStoreInfo.defineImage(COMPUTE_IMAGE, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    auto imageStore = ctx.registerExtension<lv::ImageStore>(imageStoreInfo);

    lv::ComputeShaderInfo compInfo{};
    compInfo.addImageBinding(0, [](lv::FrameContext& frame) { return &frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view; });
    compInfo.addPushConstant<float>();
    auto computeShader = ctx.registerExtension<lv::ComputeShader>("app/shaders_bin/test.comp.spv", compInfo);

    lv::RasterizerInfo rastInfo("app/shaders_bin/quad.vert.spv", "app/shaders_bin/quad.frag.spv");
    rastInfo.defineAttachment(0, [](lv::FrameContext& frame) { return frame.swapchain.vkView; });
    rastInfo.defineTexture(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view; });
    auto rasterizer = ctx.registerExtension<lv::Rasterizer>(rastInfo);

    lv::Window window(ctx, "Lovely Vulkan", 640, 480);
    while(!window.shouldClose()) {
        window.nextFrame([computeShader, rasterizer](lv::FrameContext& frame) {
            auto compFrame = frame.getExtFrame<lv::ComputeFrame>();
            auto imgStore = frame.getExtFrame<lv::ImageStoreFrame>();
            auto rastFrame = frame.getExtFrame<lv::RasterizerFrame>();

            auto barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.get(COMPUTE_IMAGE).image,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

            vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipelineLayout, 0, 1, &compFrame.descriptorSet, 0, nullptr);
            vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipeline);
            float time = static_cast<float>(glfwGetTime());
            vkCmdPushConstants(frame.commandBuffer, computeShader->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &time);
            vkCmdDispatch(frame.commandBuffer, frame.swapchain.width/16, frame.swapchain.height/16, 1);

            // Prepare the image to be sampled when rendering to the screen
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.get(COMPUTE_IMAGE).image,
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
        });
    }

    logger::info("Goodbye!");
    return 0;
};
