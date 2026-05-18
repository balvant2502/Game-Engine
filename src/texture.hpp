#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace Engine {
    class Texture {
    public:
        Texture(const vk::raii::Device& device,
                const vk::raii::PhysicalDevice& physicalDevice,
                const vk::raii::CommandPool& commandPool,
                const vk::Queue& queue,
                const std::string& filename);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        const vk::raii::Image& getImage() const;
        const vk::raii::DeviceMemory& getMemory() const;

    private:
        vk::raii::Image image = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
    };
}