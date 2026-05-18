cmake_minimum_required(VERSION 3.20)

project(GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw)

add_executable(GameEngine src/main.cpp
    src/device.hpp
    src/device.cpp
    src/game_engine.cpp
    src/game_engine.hpp
    src/glfw_window.cpp
    src/glfw_window.hpp
    src/swap_chain.hpp
    src/swap_chain.cpp
    src/graphics_pipeline.hpp
    src/graphics_pipeline.cpp
    src/file_utils.hpp
    src/static_data.hpp
    src/texture.hpp
    src/texture.cpp
    src/renderer.hpp
    src/renderer.cpp
    src/texture.hpp
    
    
    )

target_link_libraries(GameEngine 
    Vulkan::Vulkan
    glfw
)

target_include_directories(GameEngine PRIVATE
    external/glm
    external/stb
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

add_custom_command(
    TARGET GameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/shaders/shader.spv
    ${CMAKE_BINARY_DIR}/shaders/shader.spv
)

if(EXISTS "${CMAKE_SOURCE_DIR}/textures")
    file(COPY ${CMAKE_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
endif()