#include <liftedvulkan.h>

enum Images {
    COMPUTE_IMAGE,
};


int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;
    lv::AppContext ctx(info);

    auto imageStore = ctx.registerExtension<lv::ImageStore>();
    imageStore->addImage(COMPUTE_IMAGE, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT);

    lv::ComputeShaderInfo compInfo{};
    compInfo.addImageBinding(0, [](lv::FrameContext& frame) {
        return &frame.getExtFrame<lv::ImageStoreFrame>().get(COMPUTE_IMAGE).view;
    });

    auto computeShader = ctx.registerExtension<lv::ComputeShader>("app/shaders_bin/test.comp.spv", compInfo);

    lv::Window window(ctx, "Lovely Vulkan", 640, 480);

    while(!window.shouldClose()) {
        window.nextFrame([computeShader](lv::FrameContext& frame) {
            auto compFrame = frame.getExtFrame<lv::ComputeFrame>();
            auto imgStore = frame.getExtFrame<lv::ImageStoreFrame>();

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

            vkCmdDispatch(frame.commandBuffer, 5, 5, 1);


            barrier = vks::initializers::imageMemoryBarrier(
                    frame.swapchain.vkImage,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            vkCmdPipelineBarrier(
                    frame.commandBuffer,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
        });
    }

    logger::info("Goodbye!");
    return 0;
}
