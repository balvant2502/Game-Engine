#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "device.hpp"
#include "swap_chain.hpp"
#include "graphics_pipeline.hpp"
#include "texture.hpp"
#include "static_data.hpp"
#include "file_utils.hpp"
#include <vector>

namespace Engine {
    class Renderer {
    public:
        Renderer(Device &deviceObj, SwapChain &swapChain, GlfwWindow &window);
        ~Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        friend class Texture;

        void init();
        void drawFrame();
        void recreateSwapChain();
        void cleanup();

    private:
        Device &deviceObj;
        SwapChain &swapChain;
        GlfwWindow &window;
        const vk::raii::Device &device;
        const vk::raii::PhysicalDevice &physicalDevice;
        const vk::raii::SurfaceKHR &surface;
        vk::raii::CommandPool commandPool = nullptr;
        std::vector<vk::raii::CommandBuffer> commandBuffers;

        std::unique_ptr<Texture> texture;

        vk::raii::Buffer vertexBuffer = nullptr;
        vk::raii::DeviceMemory vertexBufferMemory = nullptr;
        vk::raii::Buffer indexBuffer = nullptr;
        vk::raii::DeviceMemory indexBufferMemory = nullptr;

        std::vector<vk::raii::Buffer> uniformBuffers;
        std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        // for multiple cubes
        std::vector<std::unique_ptr<UniformBufferObject>> cubeTransforms;  // Store transforms for each cube

        vk::raii::DescriptorPool descriptorPool = nullptr;
        vk::raii::DescriptorSets descriptorSets = nullptr;

        std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
        std::vector<vk::raii::Fence> inFlightFences;
        std::vector<vk::ImageLayout> swapChainImageLayouts;
        uint32_t frameIndex = 0;

        std::unique_ptr<GraphicsPipeline> graphicsPipeline;
        // Depth resources
        vk::raii::Image depthImage = nullptr;
        vk::raii::DeviceMemory depthImageMemory = nullptr;
        vk::raii::ImageView depthImageView = nullptr;

        vk::Format depthFormat = vk::Format::eD32Sfloat;

        void createDepthResources();
        vk::Format findDepthFormat();

        void createCommandPool();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createCommandBuffers();
        void recordCommandBuffer(uint32_t imageIndex);
        void createSyncObjects();
        void updateUniformBuffer(uint32_t currentImage);

        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size);
        vk::raii::CommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    };
}
