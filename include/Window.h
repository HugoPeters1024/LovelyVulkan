#pragma once
#include "precomp.h"

namespace lv {
    class Window : NoCopy {
    private:
        static void onResizeCallback(GLFWwindow* glfwWindow, int width, int height);

        int width, height;
        std::string name;
        GLFWwindow* glfwWindow;
        bool wasResized = false;
    public:
        Window();
        ~Window();

        bool shouldClose() const { return glfwWindowShouldClose(glfwWindow); }
        void processEvents() const { glfwPollEvents(); }

        int getWidth() const { return width; }
        int getHeight() const { return height; }
        const std::string &getName() const { return name; }
        GLFWwindow *getGlfwWindow() const { return glfwWindow; } };
}