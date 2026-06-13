#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace Engine {

    class Renderer;
    

    class Texture {
    public:
        Texture(const vk::raii::Device& device,
                const vk::raii::PhysicalDevice& physicalDevice,
                const vk::raii::CommandPool& commandPool,
                const vk::Queue& queue,
                Renderer& renderer,
                const std::string& filename);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        const vk::raii::Image& getImage() const;
        const vk::raii::DeviceMemory& getMemory() const;
        const vk::raii::ImageView& getTextureImageView() const;
        const vk::raii::Sampler& getTextureSampler() const;
        void createTextureImageView();
        void createTextureSampler();

    private:
        vk::raii::Image image = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        vk::raii::ImageView textureImageView = nullptr;
        // References to swapchain image views (owned by SwapChain)
        std::vector<const vk::raii::ImageView*> swapChainImageViews;
        
        // Keep references to Vulkan objects needed by helper methods
        const vk::raii::Device& device;
        const vk::raii::PhysicalDevice& physicalDevice;
        const vk::raii::CommandPool& commandPool;
        const vk::Queue& queue;
        Renderer& renderer;

        // Texture parameters
        int texWidth = 0;
        int texHeight = 0;
        int texChannels = 0;
        vk::DeviceSize imageSize = 0;
        vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;
        vk::raii::Sampler textureSampler = nullptr;
        

        // Helper methods (previously file-level static functions)
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
            vk::DeviceSize size,
            vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags properties);
        std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
            uint32_t width,
            uint32_t height,
            vk::Format format,
            vk::ImageTiling tiling,
            vk::ImageUsageFlags usage,
            vk::MemoryPropertyFlags properties);
        vk::raii::CommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);
        void transitionImageLayout(
            vk::raii::CommandBuffer& commandBuffer,
            const vk::raii::Image& image,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout);
        void copyBufferToImage(
            vk::raii::CommandBuffer& commandBuffer,
            const vk::raii::Buffer& buffer,
            vk::raii::Image& image,
            uint32_t width,
            uint32_t height);

        vk::raii::ImageView createImageView(const vk::raii::Image &image, vk::Format imageFormat);
        void createImageViews();
        
    };
}