#pragma once

#include "glfw_window.hpp"
#include "device.hpp"
#include "swap_chain.hpp"
#include "renderer.hpp"

#include <memory>
namespace Engine {

    constexpr uint32_t WIDTH = 800;
    constexpr uint32_t HEIGHT = 600;
    class GameEngine {
        private:
            GlfwWindow window;
            Device device;
            std::unique_ptr<SwapChain> swapChain;
            std::unique_ptr<Renderer> renderer;
            void initVulkan();
            void mainLoop();
        public:
            GameEngine(); 
            ~GameEngine() = default;
            GameEngine(const GameEngine&) = delete;
            GameEngine& operator=(const GameEngine&) = delete;
           void run() {
                initVulkan();
                mainLoop(); 
           }
    };
}