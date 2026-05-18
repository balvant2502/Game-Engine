#include "game_engine.hpp"

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
    }  

    void GameEngine::mainLoop()
    {
        while (!window.shouldClose())
        {
            // Poll for and process events
            glfwPollEvents();
            renderer->drawFrame();
        }
        device.getDevice().waitIdle();
    }

}