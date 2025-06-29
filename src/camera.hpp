#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.0f, PITCH = 0.0f, SPEED = 5.5f, SENSITIVITY = 0.05f, ZOOM = 60.0f;

class Camera
{
public:
    glm::vec3 Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch, MovementSpeed, MouseSensitivity, Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    glm::mat4 getViewMatrix();
    void processMovementVector(bool forward, bool backward, bool left, bool right, float deltaTime);
    void processVerticalMovement(bool up, bool down, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    glm::vec3 getPosition() const { return Position; }

private:
    void updateCameraVectors();
};
