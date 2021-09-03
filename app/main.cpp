#include <liftedvulkan.h>


int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;
    lv::AppContext ctx(info);

    lv::ComputeShaderInfo compInfo{};
    compInfo.addImageBinding(0, [](lv::FrameContext& frame) { 
        return &frame.getExtFrame<lv::ImageStoreFrame>().view; 
    });

    auto imageStore = ctx.registerExtension<lv::ImageStore>();
    auto computeShader = ctx.registerExtension<lv::ComputeShader>("app/shaders_bin/test.comp.spv", compInfo);

    lv::Window window(ctx, "Lovely Vulkan", 640, 480);

    while(!window.shouldClose()) {
        window.nextFrame([](lv::FrameContext& frame) {
            auto compFrame = frame.getExtFrame<lv::ComputeFrame>();

            auto barrier = vks::initializers::imageMemoryBarrier(
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
