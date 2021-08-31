#pragma once
#include "precomp.h"
#include "Structs.hpp"

#define MAX_FRAMES_IN_FLIGHT 3

namespace lv {
    void createWindow(AppContext& ctx, const char* name, int32_t width, int32_t height, Window& dst);
    void destroyWindow(Window& window);
    bool windowShouldClose(const Window& window);
    const FrameContext* nextFrame(Window& window);
    void getFramebufferSize(const Window& window, int32_t* width, int32_t* height);
}

