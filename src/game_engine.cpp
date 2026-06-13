#include "game_engine.hpp"
#include "camera.hpp"
#include <iostream>

// Mouse handling state (per-application simple implementation)
static double lastX = 0.0, lastY = 0.0;
static bool firstMouse = true;

// Cursor position callback — forwards movement to the renderer's camera
static void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Engine::Renderer* renderer = static_cast<Engine::Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer) return;
    Camera* cam = renderer->getCamera();
    if (!cam) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastX);
    float yoffset = static_cast<float>(lastY - ypos); // reversed: y ranges bottom->top
    lastX = xpos;
    lastY = ypos;

    cam->processMouseMovement(xoffset, yoffset, true);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Engine::Renderer* renderer = static_cast<Engine::Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer) return;
    Camera* cam = renderer->getCamera();
    if (!cam) return;
    cam->processMouseScroll(static_cast<float>(yoffset));
}

namespace Engine{
    GameEngine::GameEngine() : window{WIDTH, HEIGHT, "Vulkan Window"}, device{window}
    {
    }

   

    void GameEngine::initVulkan()
    {
        device.createDevices();
        swapChain = std::make_unique<Engine::SwapChain>(device.getPhysicalDevice(), device.getSurface(), device.getDevice(), window);
        renderer = std::make_unique<Engine::Renderer>(device, *swapChain, window);
        renderer->init();

        // Install input callbacks and capture cursor
        GLFWwindow* win = window.getWindow();
        glfwSetWindowUserPointer(win, renderer.get());
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(win, mouse_callback);
        glfwSetScrollCallback(win, scroll_callback);
    }  

    void GameEngine::mainLoop()
    {
        double lastTime = glfwGetTime();
        double lastFpsTime = glfwGetTime();
        int frames = 0;
        int fps = 0;
        while (!window.shouldClose())
        {
            // Calculate delta time
            double currentTime = glfwGetTime();
            float deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            // Poll for and process events
            glfwPollEvents();

            // Keyboard movement (polling) - WASD + space/shift for up/down
            Camera* cam = renderer->getCamera();
            frames++;

            if(currentTime - lastFpsTime >= 1.0) {
                fps = frames;
                frames = 0;
                lastFpsTime += 1.0;

                std::cout << "FPS = " << fps << std::endl;
            }
            GLFWwindow* win = window.getWindow();
            if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) cam->processKeyboard(CameraMovement::FORWARD, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) cam->processKeyboard(CameraMovement::BACKWARD, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) cam->processKeyboard(CameraMovement::LEFT, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) cam->processKeyboard(CameraMovement::RIGHT, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) cam->processKeyboard(CameraMovement::UP, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam->processKeyboard(CameraMovement::DOWN, deltaTime);
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, true);

            renderer->drawFrame();
        }
        device.getDevice().waitIdle();
    }

}