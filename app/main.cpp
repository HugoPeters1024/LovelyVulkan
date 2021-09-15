#include <liftedvulkan.h>

const lv::ImageID COMPUTE_IMAGE = lv::ImageID(0);

int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);

    lv::ImageStoreInfo imageStoreInfo;
    imageStoreInfo.defineImage(COMPUTE_IMAGE, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_GENERAL);

    lv::AppContextInfo info;
    info.registerExtension<lv::ImageStore>(imageStoreInfo);

    lv::ComputeShaderInfo compInfo{};
    compInfo.addImageBinding(0, 0, [](lv::FrameContext& frame) { return &frame.fPrev->getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view; });
    compInfo.addImageBinding(0, 1, [](lv::FrameContext& frame) { return &frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view; });
    compInfo.setPushConstantType<float>();
    info.registerExtension<lv::ComputeShader>("app/shaders_bin/test.comp.spv", compInfo);

    lv::RasterizerInfo rastInfo("app/shaders_bin/quad.vert.spv", "app/shaders_bin/quad.frag.spv");
    rastInfo.defineAttachment(0, [](lv::FrameContext& frame) { return frame.swapchain.vkView; });
    rastInfo.defineTexture(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view; });
    info.registerExtension<lv::Rasterizer>(rastInfo);

    lv::RayTracerInfo rayInfo{};
    info.registerExtension<lv::RayTracer>(rayInfo);

    lv::AppContext ctx(info);

    auto computeShader = ctx.getExtension<lv::ComputeShader>();
    auto imageStore = ctx.getExtension<lv::ImageStore>();
    auto rasterizer = ctx.getExtension<lv::Rasterizer>();

    lv::WindowInfo windowInfo;
    windowInfo.width = 640;
    windowInfo.height = 480;
    windowInfo.windowName = "Lovely Vulkan";
    lv::Window window(ctx, windowInfo);
    uint32_t tick = 0;
    while(!window.shouldClose()) {
        double ping = glfwGetTime();
        window.nextFrame([computeShader, rasterizer](lv::FrameContext& frame) {
            auto compFrame = frame.getExtFrame<lv::ComputeFrame>();
            auto imgStore = frame.getExtFrame<lv::ImageStoreFrame>();
            auto imgStorePrev = frame.fPrev->getExtFrame<lv::ImageStoreFrame>();
            auto rastFrame = frame.getExtFrame<lv::RasterizerFrame>();

            vkCmdBindDescriptorSets(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipelineLayout, 0, 1, &compFrame.descriptorSets[0], 0, nullptr);
            vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computeShader->pipeline);
            float time = static_cast<float>(glfwGetTime());
            vkCmdPushConstants(frame.commandBuffer, computeShader->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &time);
            vkCmdDispatch(frame.commandBuffer, frame.swapchain.width/16, frame.swapchain.height/16, 1);
            

            // Prepare the image to be sampled when rendering to the screen
            auto barrier = vks::initializers::imageMemoryBarrier(
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

            // Prepare the image to be sampled when rendering to the screen
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.get(COMPUTE_IMAGE).image,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
        });

        if (tick++ % 100 == 0) {
            logger::debug("fps: {}", 1.0f / (glfwGetTime()-ping));
        }
    }

    logger::info("Goodbye!");
    return 0;
}
