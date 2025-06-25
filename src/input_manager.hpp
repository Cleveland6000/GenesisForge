#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <GLFW/glfw3.h>
#include "camera.hpp"

class InputManager
{
public:
    InputManager(GLFWwindow *window, Camera &camera);
    void processInput();
    void resetMouseState();
    void processMouseMovement(double xposIn, double yposIn);

private:
    GLFWwindow *m_window;
    Camera &m_camera;
    float m_lastX, m_lastY;
    bool m_firstMouse;
};

#endif // INPUT_MANAGER_HPP
