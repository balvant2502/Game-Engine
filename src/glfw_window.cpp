#include "glfw_window.hpp"

namespace Engine
{
    GlfwWindow::GlfwWindow(int width, int height, const std::string &title) : width{width}, height{height}, title{title}
    {
        createWindow();
    }

    GlfwWindow::~GlfwWindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void GlfwWindow::createWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    }

    bool GlfwWindow::shouldClose()
    {
        return glfwWindowShouldClose(window);
    }
    GLFWwindow* GlfwWindow::getWindow() const { return window; }
} 
