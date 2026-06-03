#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include "glfw_window.hpp"
#include <assert.h>

namespace Engine
{
    class SwapChain
    {
    public:
        void createSwapChain();

        // Expose image view creation so other components can request it
        void createImageViews();

        SwapChain(const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::SurfaceKHR &surface, const vk::raii::Device &device, GlfwWindow &window);
        ~SwapChain();
        SwapChain(const SwapChain &) = delete;
        SwapChain &operator=(const SwapChain &) = delete;
        // Accessors
        const std::vector<vk::raii::ImageView>& getImageViews() const;
        const vk::Extent2D& getExtent() const;
        const vk::SurfaceFormatKHR& getSurfaceFormat() const;
        const vk::raii::SwapchainKHR& getSwapchain() const;
        const std::vector<vk::Image>& getImages() const;
        

    private:
        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> swapChainImages;
        vk::SurfaceFormatKHR swapChainSurfaceFormat;
        vk::Extent2D swapChainExtent;
        std::vector<vk::raii::ImageView> swapChainImageViews;
        const vk::raii::PhysicalDevice &physicalDevice;
        const vk::raii::SurfaceKHR &surface;
        const vk::raii::Device &device;
        GlfwWindow &window;

            
        void makeSwapChain();
        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
        uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);
    };
}