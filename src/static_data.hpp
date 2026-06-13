#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;
	

	static vk::VertexInputBindingDescription getBindingDescription()
	{
       vk::VertexInputBindingDescription desc{};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = vk::VertexInputRate::eVertex;
		return desc;
	}

		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			vk::VertexInputAttributeDescription desc1{};
				desc1.location = 0;
				desc1.binding = 0;
				desc1.format = vk::Format::eR32G32B32Sfloat;
				desc1.offset = offsetof(Vertex, pos);
				vk::VertexInputAttributeDescription desc2{};
				desc2.location = 1;
				desc2.binding = 0;
				desc2.format = vk::Format::eR32G32B32Sfloat;
				desc2.offset = offsetof(Vertex, color);
				vk::VertexInputAttributeDescription desc3{};
				desc3.location = 2;
				desc3.binding = 0;
				desc3.format = vk::Format::eR32G32Sfloat;
				desc3.offset = offsetof(Vertex, uv);
				std::array<vk::VertexInputAttributeDescription, 3> desc = { desc1, desc2, desc3 };
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
	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 0  0 (back - red)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 1  1 (left - blue)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // 2  2 (bottom - cyan)
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // 3  3 (back - red)
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 4  4 (right - yellow)
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 5  5 (bottom - cyan)
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // 6  6 (back - red)
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // 7  7 (right - yellow)
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 8  8 (top - magenta)
	{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 9  9 (back - red)
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 10 10 (left - blue)
	{{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 11 11 (top - magenta)
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 12 12 (front - green)
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 13 13 (left - blue)
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // 14 14 (bottom - cyan)
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 15 15 (front - green)
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 16 16 (right - yellow)
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 17 17 (bottom - cyan)
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // 18 18 (front - green)
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // 19 19 (right - yellow)
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 20 20 (top - magenta)
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // 21 21 (front - green)
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // 22 22 (left - blue)
	{{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}  // 23  23 (top - magenta)
};

inline const std::vector<uint16_t> indices = {
	// back face (z = -0.5)
	0, 3, 6, 6, 9, 0,
	// front face (z = +0.5)
	12, 15, 18, 18, 21, 12,
	// left face (x = -0.5)
	1, 10, 22, 22, 13, 1,
	// right face (x = +0.5)
	4, 16, 19, 19, 7, 4,
	// top face (y = +0.5)
	11, 8, 20, 20, 23, 11,
	// bottom face (y = -0.5)
	2, 14, 17, 17, 5, 2
};

