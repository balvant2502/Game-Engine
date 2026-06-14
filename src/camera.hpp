#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
    private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    void updateCameraVectors();

    public:
    Camera(
        glm::vec3 position = glm::vec3(2.0f, 2.0f, 2.0f),  // start slightly away from origin
        glm::vec3 up = glm::vec3(0.0f, 0.0f, -1.0f),        // Z-axis as world up for this project
        float yaw = -135.0f,                                // angled toward origin from (2,2,2)
        float pitch = -35.26438968f                         // approximate pitch to look toward origin
    );

    Camera(const Camera &) = delete;
    Camera operator=(const Camera&) = delete;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;

        
    void processKeyboard(CameraMovement direction, float deltaTime);     
    void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);  
    void processMouseScroll(float yOffset);                              

    // Property access methods for external systems
    // Provide controlled access to internal state without exposing implementation details
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    float getZoom() const { return zoom; }


};
