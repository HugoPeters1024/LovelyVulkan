#include <liftedvulkan.h>

int main(int argc, char** argv) {
    logger::set_level(spdlog::level::debug);
    lv::AppContextInfo info;
    lv::AppContext ctx;
    lv::createAppContext(info, ctx);

    lv::Window window;
    lv::createWindow(ctx, "Lovely Vulkan", 640, 480, window);

    while(!lv::windowShouldClose(window)) {
        auto frameContext = lv::nextFrame(window);
    }

    logger::info("Goodbye!");
    lv::destroyWindow(window);
    lv::destroyAppContext(ctx);
    return 0;
}
