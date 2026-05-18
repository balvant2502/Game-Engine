#include "renderer.hpp"
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

    Renderer::Renderer(Device &deviceObj, SwapChain &swapChain, GlfwWindow &window) : deviceObj{deviceObj}, swapChain{swapChain}, window{window}, device{deviceObj.getDevice()}, physicalDevice{deviceObj.getPhysicalDevice()}, surface{deviceObj.getSurface()} {
    }

    void Renderer::init() {
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device, swapChain.getExtent(), swapChain.getSurfaceFormat());
        swapChainImageLayouts.assign(swapChain.getImages().size(), vk::ImageLayout::eUndefined);
        createCommandPool();
        texture = std::make_unique<Texture>(
            device,
            physicalDevice,
            commandPool,
            deviceObj.getQueue(),
            "textures/brick.jpg");
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void Renderer::createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = deviceObj.getQueueIndex();
        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        vk::raii::Buffer buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(*bufferMemory, 0);
        return {std::move(buffer), std::move(bufferMemory)};
    }

    void Renderer::createVertexBuffer() {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(dataStaging, vertices.data(), bufferSize);
        stagingBufferMemory.unmapMemory();
        std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    }

    void Renderer::createIndexBuffer() {
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void *data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, indices.data(), (size_t)bufferSize);
        stagingBufferMemory.unmapMemory();
        std::tie(indexBuffer, indexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    }

    void Renderer::createUniformBuffers() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
            auto [buffer, bufferMem] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMemory.emplace_back(std::move(bufferMem));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
        }
    }

    void Renderer::createDescriptorPool() {
        vk::DescriptorPoolSize poolSize{};
        poolSize.type = vk::DescriptorType::eUniformBuffer;
        poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void Renderer::createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *graphicsPipeline->getDescriptorSetLayout());
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();
        descriptorSets = vk::raii::DescriptorSets(device, allocInfo);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = *uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            vk::WriteDescriptorSet descriptorWrite{};
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            std::array<vk::WriteDescriptorSet, 1> writes = {descriptorWrite};
            device.updateDescriptorSets(writes, nullptr);
        }
    }

    void Renderer::createCommandBuffers() {
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void Renderer::recordCommandBuffer(uint32_t imageIndex) {
        auto &commandBuffer = commandBuffers[frameIndex];
        vk::CommandBufferBeginInfo beginInfo{};
        commandBuffer.begin(beginInfo);

        vk::ImageMemoryBarrier imageToColorBarrier{};
        imageToColorBarrier.oldLayout = swapChainImageLayouts[imageIndex];
        imageToColorBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        imageToColorBarrier.srcAccessMask = {};
        imageToColorBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        imageToColorBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageToColorBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageToColorBarrier.image = swapChain.getImages()[imageIndex];
        imageToColorBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageToColorBarrier.subresourceRange.baseMipLevel = 0;
        imageToColorBarrier.subresourceRange.levelCount = 1;
        imageToColorBarrier.subresourceRange.baseArrayLayer = 0;
        imageToColorBarrier.subresourceRange.layerCount = 1;
        std::array<vk::ImageMemoryBarrier, 1> imageToColorBarriers = {imageToColorBarrier};
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            {},
            nullptr,
            nullptr,
            imageToColorBarriers);

        vk::ClearValue clearColor{};
        clearColor.color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
        vk::RenderingAttachmentInfo attachmentInfo{};
        attachmentInfo.imageView = swapChain.getImageViews()[imageIndex];
        attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        attachmentInfo.clearValue = clearColor;
        vk::RenderingInfo renderingInfo{};
        vk::Rect2D renderArea{};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent = swapChain.getExtent();
        renderingInfo.renderArea = renderArea;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &attachmentInfo;
        commandBuffer.beginRendering(renderingInfo);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->getPipeline());
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(swapChain.getExtent().height);
        viewport.width = static_cast<float>(swapChain.getExtent().width);
        viewport.height = -static_cast<float>(swapChain.getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        commandBuffer.setViewport(0, viewport);
        vk::Rect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent = swapChain.getExtent();
        commandBuffer.setScissor(0, scissor);
        commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
        commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
        std::array<vk::DescriptorSet, 1> sets = {descriptorSets[frameIndex]};
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->getPipelineLayout(), 0, sets, {});
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        commandBuffer.endRendering();

        vk::ImageMemoryBarrier imageToPresentBarrier{};
        imageToPresentBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        imageToPresentBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        imageToPresentBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        imageToPresentBarrier.dstAccessMask = {};
        imageToPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageToPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageToPresentBarrier.image = swapChain.getImages()[imageIndex];
        imageToPresentBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageToPresentBarrier.subresourceRange.baseMipLevel = 0;
        imageToPresentBarrier.subresourceRange.levelCount = 1;
        imageToPresentBarrier.subresourceRange.baseArrayLayer = 0;
        imageToPresentBarrier.subresourceRange.layerCount = 1;
        std::array<vk::ImageMemoryBarrier, 1> imageToPresentBarriers = {imageToPresentBarrier};
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            {},
            nullptr,
            nullptr,
            imageToPresentBarriers);

        swapChainImageLayouts[imageIndex] = vk::ImageLayout::ePresentSrcKHR;
        commandBuffer.end();
    }

    void Renderer::createSyncObjects() {
        assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());
        for (size_t i = 0; i < swapChain.getImages().size(); i++) {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            presentCompleteSemaphores.emplace_back(device, semaphoreInfo);
            vk::FenceCreateInfo fenceInfo{};
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            inFlightFences.emplace_back(device, fenceInfo);
        }
    }

    void Renderer::updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void Renderer::drawFrame() {
        auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
        if (fenceResult != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for fence!");
        }
        auto [result, imageIndex] = swapChain.getSwapchain().acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        updateUniformBuffer(frameIndex);
        device.resetFences(*inFlightFences[frameIndex]);
        commandBuffers[frameIndex].reset();
        recordCommandBuffer(imageIndex);
        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo{};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &*presentCompleteSemaphores[frameIndex];
        submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffers[frameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &*renderFinishedSemaphores[imageIndex];
        deviceObj.getQueue().submit(submitInfo, *inFlightFences[frameIndex]);
        vk::PresentInfoKHR presentInfoKHR{};
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores = &*renderFinishedSemaphores[imageIndex];
        presentInfoKHR.swapchainCount = 1;
        presentInfoKHR.pSwapchains = &*swapChain.getSwapchain();
        presentInfoKHR.pImageIndices = &imageIndex;
        result = deviceObj.getQueue().presentKHR(presentInfoKHR);
        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR)) {
            recreateSwapChain();
        }
        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window.getWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window.getWindow(), &width, &height);
            glfwWaitEvents();
        }
        device.waitIdle();
        // recreate swap chain
        swapChain.createSwapChain();
        swapChainImageLayouts.assign(swapChain.getImages().size(), vk::ImageLayout::eUndefined);
        // re-create pipeline
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device, swapChain.getExtent(), swapChain.getSurfaceFormat());
        createCommandBuffers();
    }

    void Renderer::cleanup() {
        // resources will be freed by RAII members
    }

    void Renderer::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size) {
        vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
        vk::BufferCopy copyRegion{};
        copyRegion.size = size;
        commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, copyRegion);
        endSingleTimeCommands(std::move(commandCopyBuffer));
    }

    vk::raii::CommandBuffer Renderer::beginSingleTimeCommands() {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;
        vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        commandBuffer.begin(beginInfo);
        return std::move(commandBuffer);
    }

    void Renderer::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer) {
        commandBuffer.end();
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;
        deviceObj.getQueue().submit(submitInfo, nullptr);
        deviceObj.getQueue().waitIdle();
    }

    uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

}
