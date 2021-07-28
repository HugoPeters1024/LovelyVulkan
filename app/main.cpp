#include <cstdio>

#include <liftedvulkan.h>

int main(int argc, char** argv) {
    lv::DeviceInfo deviceInfo;
    lv::Device device(deviceInfo);
    auto window = device.createWindow("Testbed", 640, 480);

    while (!window->shouldClose()) {
        window->processEvents();
    }
    return 0;
}