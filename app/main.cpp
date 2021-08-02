#include <liftedvulkan.h>

int main(int argc, char** argv) {
    lv::DeviceInfo deviceInfo;
    lv::Device device(deviceInfo);
    auto window = device.createWindow("Testbed", 640, 480);

    lv::AttachmentView attachment{};
    lv::RenderPassCore renderPass(device);
    renderPass.bindToWindow(window)
              .addAttachment(&attachment, VK_FORMAT_R8G8B8A8_SNORM, 0)
              .build();

    while (!window->shouldClose()) {
        uint32_t imageIdx = window->startFrame();
        std::vector<VkCommandBuffer> commands;
        window->submitFrame(commands, imageIdx);
    }

    return 0;
}