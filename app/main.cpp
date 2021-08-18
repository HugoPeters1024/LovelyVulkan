#include <liftedvulkan.h>

int main(int argc, char** argv) {
    lv::DeviceInfo deviceInfo;
    lv::Device device(deviceInfo);
    auto window = device.createWindow("Testbed", 640, 480);

    lv::AttachmentView attachment{};
    lv::RenderPassCore renderPass(device);
    renderPass.bindToWindow(window)
              .addWindowAttachment(&attachment, window, 0)
              .build();

    lv::ComputeShaderCore core;
    core.duplication = window->getNrImages();
    core.filename = "app/shaders_bin/test.comp.spv";

    lv::ComputeShader compute(device, core);
    lv::ComputeShaderBindingSet bindingSet;

    compute.createBindingSet(&bindingSet);
    bindingSet.bindImage(0, window->get)



    while (!window->shouldClose()) {
        uint32_t imageIdx = window->startFrame();
        std::vector<VkCommandBuffer> commands;
        window->submitFrame(commands, imageIdx);
    }

    return 0;
}