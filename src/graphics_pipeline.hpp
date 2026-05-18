#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "static_data.hpp"


namespace Engine{
    class GraphicsPipeline {
        public:
            GraphicsPipeline(const vk::raii::Device &device, const vk::Extent2D &swapChainExtent, const vk::SurfaceFormatKHR &surfaceFormat);
            ~GraphicsPipeline() = default;
            GraphicsPipeline(const GraphicsPipeline &) = delete;
            GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
            const vk::raii::Pipeline& getPipeline() const;
            const vk::raii::PipelineLayout& getPipelineLayout() const;
            const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() const;
            void createPipeline();

        private:
            void createDescriptorSetLayout();
            void createGraphicsPipeline();
            const vk::raii::Device &device;
            const vk::Extent2D &swapChainExtent;
            const vk::SurfaceFormatKHR swapChainSurfaceFormat;
            // no render pass required (using dynamic rendering)
            
            vk::raii::PipelineLayout pipelineLayout = nullptr;
            vk::raii::Pipeline graphicsPipeline = nullptr;
            vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
            [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
        
    };
}