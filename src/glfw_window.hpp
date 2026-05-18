#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Engine {
    class GlfwWindow {
        private:
            GLFWwindow* window;
            int width;
            int height;
            std::string title;
        public:
        GlfwWindow(int width, int height, const std::string &title);
        ~GlfwWindow();

        GlfwWindow(const GlfwWindow&) = delete;
        GlfwWindow& operator=(const GlfwWindow&) = delete;
        void createWindow();
        bool shouldClose();
        GLFWwindow* getWindow() const;

       
    };
}