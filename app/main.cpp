#include <cstdio>

#include <liftedvulkan.h>

int main(int argc, char** argv) {
    uchar lol;
    lv::Window window;

    while (!window.shouldClose()) {
        window.processEvents();
    }
    printf("Hello Vulkan!\n");
}