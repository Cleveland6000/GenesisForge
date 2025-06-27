#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(0.0f, 0.0f, -1.0f), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM),
      Position(position), WorldUp(up), Yaw(yaw), Pitch(pitch)
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::processMovementVector(bool forward, bool backward, bool left, bool right, float deltaTime)
{
    glm::vec3 movement(0.0f);
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    if (forward)
        movement += horizontalFront;
    if (backward)
        movement -= horizontalFront;
    if (left)
        movement -= Right;
    if (right)
        movement += Right;
    if (glm::length(movement) > 0.0001f)
        movement = glm::normalize(movement);
    Position += movement * MovementSpeed * deltaTime;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;
    Yaw += xoffset;
    Pitch += yoffset;
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::processVerticalMovement(bool up, bool down, float deltaTime)
{
    if (up)
        Position += WorldUp * MovementSpeed * deltaTime * 2.0f;
    if (down)
        Position -= WorldUp * MovementSpeed * deltaTime * 2.0f;
}
