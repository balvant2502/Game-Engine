#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
       vk::VertexInputBindingDescription desc{};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = vk::VertexInputRate::eVertex;
		return desc;
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
      vk::VertexInputAttributeDescription desc1{};
		desc1.location = 0;
		desc1.binding = 0;
		desc1.format = vk::Format::eR32G32Sfloat;
		desc1.offset = offsetof(Vertex, pos);
		vk::VertexInputAttributeDescription desc2{};
		desc2.location = 1;
		desc2.binding = 0;
		desc2.format = vk::Format::eR32G32B32Sfloat;
		desc2.offset = offsetof(Vertex, color);
		std::array<vk::VertexInputAttributeDescription, 2> desc = { desc1, desc2 };
		return desc;
	}
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

inline constexpr int MAX_FRAMES_IN_FLIGHT = 2;

inline const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}} };

inline const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};
