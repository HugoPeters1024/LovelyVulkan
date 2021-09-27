#include <liftedvulkan.h>

const lv::ImageID COMPUTE_IMAGE = lv::ImageID(0);

int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;

    lv::ImageStoreInfo imageStoreInfo;
    imageStoreInfo.defineStaticImage(lv::ImageID(1), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_GENERAL);
    info.registerExtension<lv::ImageStore>(imageStoreInfo);

    lv::RayTracerInfo rayInfo{};
    lv::Mesh sibenik, bunny;
    sibenik.load("./app/sibenik/sibenik.obj");
    bunny.load("./app/bunny.obj");
    rayInfo.meshes.push_back(&sibenik);
    rayInfo.meshes.push_back(&bunny);
    info.registerExtension<lv::RayTracer>(rayInfo);

    lv::RasterizerInfo rastInfo("app/shaders_bin/quad.vert.spv", "app/shaders_bin/quad.frag.spv");
    rastInfo.defineAttachment(0, [](lv::FrameContext& frame) { return frame.swapchain.vkView; });
    rastInfo.defineTexture(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ImageStoreFrame>().getStatic(lv::ImageID(1))->view; });
    info.registerExtension<lv::Rasterizer>(rastInfo);


    lv::AppContext ctx(info);

    auto imageStore = ctx.getExtension<lv::ImageStore>();
    auto rasterizer = ctx.getExtension<lv::Rasterizer>();
    auto raytracer = ctx.getExtension<lv::RayTracer>();

    lv::WindowInfo windowInfo;
    windowInfo.width = 1280;
    windowInfo.height = 768;
    windowInfo.windowName = "Lovely Vulkan";
    lv::Window window(ctx, windowInfo);

    lv::Camera camera{window.glfwWindow};

    uint32_t tick = 0;
    double ping = glfwGetTime();
    while(!window.shouldClose()) {
        float dt = glfwGetTime() - ping;
        ping = glfwGetTime();

        if (tick++ % 100 == 0) {
            logger::debug("fps: {}", 1.0f / dt);
        }

        window.nextFrame([&](lv::FrameContext& frame) {
            auto imgStore = frame.getExtFrame<lv::ImageStoreFrame>();
            auto rastFrame = frame.getExtFrame<lv::RasterizerFrame>();

            // Run the raytracer
            camera.update(dt);
            if (camera.getHasMoved()) raytracer->resetAccumulator();
            raytracer->render(frame, camera);

            // Prepare the image to be sampled when rendering to the screen
            auto barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.getStatic(lv::ImageID(1))->image,
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


            // Set the image back for ray tracing
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.getStatic(lv::ImageID(1))->image,
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

    }

    logger::info("Goodbye!");
    return 0;
}
