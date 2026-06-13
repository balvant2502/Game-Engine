#include "renderer.hpp"
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{

    Renderer::Renderer(Device &deviceObj, SwapChain &swapChain, GlfwWindow &window) : deviceObj{deviceObj}, swapChain{swapChain}, window{window}, device{deviceObj.getDevice()}, physicalDevice{deviceObj.getPhysicalDevice()}, surface{deviceObj.getSurface()}
    {
    }

    void Renderer::init()
    {
        createCommandPool();
        createDepthResources();
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device, swapChain.getExtent(), swapChain.getSurfaceFormat(), depthFormat);
        swapChainImageLayouts.assign(swapChain.getImages().size(), vk::ImageLayout::eUndefined);
        texture = std::make_unique<Texture>(
            device,
            physicalDevice,
            commandPool,
            deviceObj.getQueue(),
            *this,
            "textures/brick.jpg");
        // Populate texture references to the current swapchain image views
        
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void Renderer::createCommandPool()
    {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = deviceObj.getQueueIndex();
        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
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

    void Renderer::createVertexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(dataStaging, vertices.data(), bufferSize);
        stagingBufferMemory.unmapMemory();
        std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    }

    void Renderer::createIndexBuffer()
    {
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        void *data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, indices.data(), (size_t)bufferSize);
        stagingBufferMemory.unmapMemory();
        std::tie(indexBuffer, indexBufferMemory) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    }

    void Renderer::createUniformBuffers()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

            //for cube1 
            auto [buffer1, bufferMem1] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uniformBuffers.emplace_back(std::move(buffer1));
            uniformBuffersMemory.emplace_back(std::move(bufferMem1));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));

            // for cube2
            auto [buffer2, bufferMem2] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            uniformBuffers.emplace_back(std::move(buffer2));
            uniformBuffersMemory.emplace_back(std::move(bufferMem2));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
        }
    }

    void Renderer::createDescriptorPool()
    {
        vk::DescriptorPoolSize poolSize1{};
        poolSize1.type = vk::DescriptorType::eUniformBuffer;
        poolSize1.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;

        vk::DescriptorPoolSize poolSize2{};
        poolSize2.type = vk::DescriptorType::eCombinedImageSampler;
        poolSize2.descriptorCount =  MAX_FRAMES_IN_FLIGHT * 2;

        std::array<vk::DescriptorPoolSize, 2> poolSize = {poolSize1, poolSize2};

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * 2;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
        poolInfo.pPoolSizes = poolSize.data();
        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void Renderer::createDescriptorSets()
    {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT * 2, *graphicsPipeline->getDescriptorSetLayout());
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT * 2;
        allocInfo.pSetLayouts = layouts.data();
        descriptorSets = vk::raii::DescriptorSets(device, allocInfo);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT * 2; i++)
        {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = *uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorImageInfo imageInfo {};
            imageInfo.sampler = texture->getTextureSampler();
            imageInfo.imageView = texture->getTextureImageView();
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal; 


            vk::WriteDescriptorSet descriptorWrite1{};
            descriptorWrite1.dstSet = descriptorSets[i];
            descriptorWrite1.dstBinding = 0;
            descriptorWrite1.dstArrayElement = 0;
            descriptorWrite1.descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrite1.descriptorCount = 1;
            descriptorWrite1.pBufferInfo = &bufferInfo;

            vk::WriteDescriptorSet descriptorWrite2{};
            descriptorWrite2.dstSet = descriptorSets[i];
            descriptorWrite2.dstBinding = 1;
            descriptorWrite2.dstArrayElement = 0;
            descriptorWrite2.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrite2.descriptorCount = 1;
            descriptorWrite2.pImageInfo = &imageInfo;

            std::array<vk::WriteDescriptorSet, 2> writes = {descriptorWrite1, descriptorWrite2};


            device.updateDescriptorSets(writes, nullptr);
        }
    }

    void Renderer::createCommandBuffers()
    {
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void Renderer::recordCommandBuffer(uint32_t imageIndex)
    {
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


        // attach depth buffer
        vk::RenderingAttachmentInfo depthAttachmentInfo{};
        depthAttachmentInfo.imageView = *depthImageView;
        depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
        vk::ClearValue depthClear{};
        depthClear.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
        depthAttachmentInfo.clearValue = depthClear;
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;

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

        // draw cube1
        std::array<vk::DescriptorSet, 1> sets1 = {descriptorSets[frameIndex * 2]};
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->getPipelineLayout(), 0, sets1, {});
        commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        // draw cube2
        std::array<vk::DescriptorSet, 1> sets2 = {descriptorSets[frameIndex * 2 + 1]};
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline->getPipelineLayout(), 0, sets2, {});
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

    void Renderer::createSyncObjects()
    {
        assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());
        for (size_t i = 0; i < swapChain.getImages().size(); i++)
        {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            presentCompleteSemaphores.emplace_back(device, semaphoreInfo);
            vk::FenceCreateInfo fenceInfo{};
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            inFlightFences.emplace_back(device, fenceInfo);
        }
    }

    void Renderer::updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(currentTime - startTime).count();

        // CUBE 1
        UniformBufferObject ubo1{};
        // Rotate over time (radians). Use distinct axis angles and apply time as the angle multiplier.
        float angleX = time * glm::radians(45.0f);
        float angleY = time * glm::radians(45.0f);
        float angleZ = time * glm::radians(45.0f);

        glm::mat4 rotX1 = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY1 = glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ1 = glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 rotationMatrix = rotY1 * rotZ1 * rotX1;
        glm::mat4 translation1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        ubo1.model = translation1 * rotationMatrix;

        // CUBE 2 - position to the right, rotates differently
        UniformBufferObject ubo2{};
        float angleX2 = time * glm::radians(45.0f);
        float angleY2 = time * glm::radians(45.0f);
        float angleZ2 = time * glm::radians(45.0f);

        glm::mat4 rotX2 = glm::rotate(glm::mat4(1.0f), angleX2, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY2 = glm::rotate(glm::mat4(1.0f), angleY2, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ2 = glm::rotate(glm::mat4(1.0f), angleZ2, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 rotationMatrix2 = rotY2 * rotZ2 * rotX2;

        glm::mat4 translation2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Move right
        ubo2.model = translation2 * rotationMatrix2;

        // Obtain view/projection from the free camera
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height), 0.1f, 10.0f);
        // GLM's perspective is defined for OpenGL; invert Y for Vulkan NDC
        proj[1][1] *= -1;

        ubo1.view = view;
        ubo2.view = view;
        ubo1.proj = proj;
        ubo2.proj = proj;

        void* mapped1 = uniformBuffersMapped[currentImage * 2];
        void* mapped2 = uniformBuffersMapped[currentImage * 2 + 1];
        memcpy(mapped1, &ubo1, sizeof(ubo1));
        memcpy(mapped2, &ubo2, sizeof(ubo2));
    }

    void Renderer::drawFrame()
    {
        auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
        if (fenceResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to wait for fence!");
        }
        auto [result, imageIndex] = swapChain.getSwapchain().acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
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
        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR))
        {
            recreateSwapChain();
        }
        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::recreateSwapChain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window.getWindow(), &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window.getWindow(), &width, &height);
            glfwWaitEvents();
        }
        device.waitIdle();
        // recreate swap chain
        swapChain.createSwapChain();
        // Update texture references to new swapchain image views
        
        swapChainImageLayouts.assign(swapChain.getImages().size(), vk::ImageLayout::eUndefined);
        // recreate depth resources for new extent
        createDepthResources();
        // re-create pipeline
        graphicsPipeline = std::make_unique<GraphicsPipeline>(device, swapChain.getExtent(), swapChain.getSurfaceFormat(), depthFormat);
        createCommandBuffers();
    }

    void Renderer::cleanup()
    {
        // resources will be freed by RAII members
    }

    void Renderer::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
    {
        vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
        vk::BufferCopy copyRegion{};
        copyRegion.size = size;
        commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, copyRegion);
        endSingleTimeCommands(std::move(commandCopyBuffer));
    }

    vk::Format Renderer::findDepthFormat()
    {
        std::vector<vk::Format> candidates = {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
        for (auto format : candidates)
        {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);
            if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported depth format!");
    }

    void Renderer::createDepthResources()
    {
        depthFormat = findDepthFormat();
        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = swapChain.getExtent().width;
        imageInfo.extent.height = swapChain.getExtent().height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        depthImage = vk::raii::Image(device, imageInfo);

        vk::MemoryRequirements memRequirements = depthImage.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        depthImageMemory = vk::raii::DeviceMemory(device, allocInfo);
        depthImage.bindMemory(*depthImageMemory, 0);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = *depthImage;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        depthImageView = vk::raii::ImageView(device, viewInfo);

        // Transition layout to depth attachment optimal
        vk::raii::CommandBuffer cmd = beginSingleTimeCommands();
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = *depthImage;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests, {}, nullptr, nullptr, {barrier});
        endSingleTimeCommands(std::move(cmd));
    }

    vk::raii::CommandBuffer Renderer::beginSingleTimeCommands()
    {
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

    void Renderer::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
    {
        commandBuffer.end();
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;
        deviceObj.getQueue().submit(submitInfo, nullptr);
        deviceObj.getQueue().waitIdle();
    }

    uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

}
