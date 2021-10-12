#include <liftedvulkan.h>

const uint32_t WINDOW_WIDTH = 1024;
const uint32_t WINDOW_HEIGHT = 512;

int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);

    lv::AppContextInfo info;
    info.registerExtension<lv::ResourceStore>();
    info.registerExtension<lv::RayTracer>();
    info.registerExtension<lv::Rasterizer>();
    info.registerExtension<lv::Overlay>();
    info.registerExtension<lv::ComputeShader>();
    lv::AppContext ctx(info);


    lv::ResourceStoreInfo resourceStoreInfo;
    resourceStoreInfo.defineStaticImage(1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_GENERAL);
    resourceStoreInfo.defineBuffer(0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(float));
    auto& imageStore = ctx.addExtension<lv::ResourceStore>(ctx, resourceStoreInfo);

    lv::RasterizerInfo rastInfo("app/shaders_bin/quad.vert.spv", "app/shaders_bin/quad.frag.spv");
    rastInfo.defineAttachment(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::WindowFrame>().vkView; });
    rastInfo.defineTexture(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ResourceFrame>().getStatic(1)->view; });
    auto& rasterizer = ctx.addExtension<lv::Rasterizer>(ctx, rastInfo);

    lv::RayTracerInfo rayInfo{};
    lv::Mesh sibenik, bunny;
    bunny.load("./app/cube.obj");
    bunny.indices.resize(3);
    bunny.normals.resize(3);
    sibenik.load("./app/sibenik/sibenik.obj");
    rayInfo.meshes.push_back(&sibenik);
    rayInfo.meshes.push_back(&bunny);
    auto& raytracer = ctx.addExtension<lv::RayTracer>(ctx, rayInfo);

    lv::ComputeShaderInfo sumImageInfo{};
    sumImageInfo.addBufferBinding(0, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ResourceFrame>().getBuffer(0).buffer; });
    sumImageInfo.addImageBinding(1, [](lv::FrameContext& frame) { return frame.getExtFrame<lv::ResourceFrame>().getStatic(1)->view; });
    auto& sumImage = ctx.addExtension<lv::ComputeShader>(ctx, "./app/shaders_bin/sumImage.comp.spv", sumImageInfo);

    lv::WindowInfo windowInfo;
    windowInfo.width = 1280;
    windowInfo.height = 768;
    windowInfo.windowName = "Lovely Vulkan";
    auto& window = ctx.addFrameManager<lv::Window>(ctx, windowInfo);

    lv::Camera camera{window.getGLFWwindow()};
    camera.eye = glm::vec3(0, -5, 0);

    lv::OverlayInfo overlayInfo{};
    overlayInfo.frameManager = &window;
    overlayInfo.glfwWindow = window.getGLFWwindow();
    overlayInfo.renderPass = rasterizer.getRenderPass();
    auto& overlay = ctx.addExtension<lv::Overlay>(ctx, overlayInfo);

    uint32_t tick = 0;
    double ping = glfwGetTime();
    float fps = 0.0f;
    while(!window.shouldClose()) {

        window.nextFrame([&](lv::FrameContext& frame) {
            auto& imgStore = frame.getExtFrame<lv::ResourceFrame>();
            auto& rastFrame = frame.getExtFrame<lv::RasterizerFrame>();
            auto& sumImageFrame = frame.getExtFrame<lv::ComputeFrame>();

            float dt = glfwGetTime() - ping;
            ping = glfwGetTime();

            imgStore.getBuffer(0).getData<float>()[0] = 0;

            // Run the raytracer
            camera.update(dt);
            if (camera.getHasMoved()) raytracer.resetAccumulator();
            raytracer.render(frame, camera, overlay.NEE);
            
            // Collect info about the amount of energy
            vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, sumImage.pipeline);
            vkCmdBindDescriptorSets(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, sumImage.pipelineLayout, 0, 1, &sumImageFrame.descriptorSet, 0, nullptr);
            vkCmdDispatch(frame.cmdBuffer, WINDOW_WIDTH / 64, WINDOW_HEIGHT / 64, 1);

            // Prepare the image to be sampled when rendering to the screen
            auto barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.getStatic(1)->image,
                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            vkCmdPipelineBarrier(
                    frame.cmdBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

            rasterizer.startPass(frame);
            vkCmdDraw(frame.cmdBuffer, 3, 1, 0, 0);

            fps = 0.9f * fps + 0.1f * (1.0f / dt);
            overlay.render(frame, *frame.fPrev->getExtFrame<lv::ResourceFrame>().getBuffer(0).getData<float>(), fps);
            rasterizer.endPass(frame);


            // Set the image back for ray tracing
            barrier = vks::initializers::imageMemoryBarrier(
                    imgStore.getStatic(1)->image,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPipelineBarrier(
                    frame.cmdBuffer,
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
