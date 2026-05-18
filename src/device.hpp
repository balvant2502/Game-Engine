#pragma once
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <iostream>
#include "glfw_window.hpp"

namespace Engine
{

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

inline const std::vector<char const *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

inline std::vector<const char *> requiredDeviceExtension = {
    vk::KHRSwapchainExtensionName};


    class Device
    {
    private:
        vk::raii::Context context;
        vk::raii::Instance instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
        vk::raii::SurfaceKHR surface = nullptr;
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        GlfwWindow &window;
        vk::raii::Device device = nullptr;
        uint32_t queueIndex = ~0;
        vk::raii::Queue queue = nullptr;

        void createInstance();
        std::vector<const char *> getRequiredInstanceExtensions();
        void setupDebugMessenger();
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *);
        void createSurface();
        void pickPhysicalDevice();
        bool isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice);
        void createLogicalDevice();

    public:
        Device(GlfwWindow &window);
        ~Device();
        Device(const Device &) = delete;
        Device &operator=(const Device &) = delete;
        const vk::raii::PhysicalDevice& getPhysicalDevice() const;
        const vk::raii::SurfaceKHR& getSurface() const;
        const vk::raii::Device& getDevice() const;
        const vk::raii::Queue& getQueue() const;
        uint32_t getQueueIndex() const;
        
        void createDevices(); 
    };

}