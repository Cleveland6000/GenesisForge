#include "input_manager.hpp"
#include <iostream> // デバッグ用に一時的に追加。最終的には不要であれば削除。

InputManager::InputManager(Camera& camera)
    : m_window(nullptr), m_camera(camera), m_firstMouse(true), m_lastX(0.0f), m_lastY(0.0f) {}

void InputManager::setWindow(GLFWwindow* window) {
    m_window = window;
    if (m_window) {
        // カーソルを非表示にし、中心に固定する
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        m_lastX = static_cast<float>(width) / 2.0f;
        m_lastY = static_cast<float>(height) / 2.0f;
        m_firstMouse = true; 
        std::cout << "InputManager: Window set and cursor disabled.\n"; 
    } else {
        std::cerr << "InputManager: Attempted to set null window.\n";
    }
}

void InputManager::processInput() {
    // この関数は主にキーボード入力処理を行うはず
}

void InputManager::processMouseMovement(double xposIn, double yposIn) {
    if (!m_window) {
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastX;
    float yoffset = m_lastY - ypos; // 逆Y座標
    m_lastX = xpos;
    m_lastY = ypos;

    // カメラにマウス移動を伝える
    m_camera.processMouseMovement(xoffset, yoffset); 
}

void InputManager::resetMouseState() {
    m_firstMouse = true;
    if (m_window) {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        glfwSetCursorPos(m_window, static_cast<double>(width) / 2.0, static_cast<double>(height) / 2.0);
    }
}