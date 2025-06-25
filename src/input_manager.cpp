// input_manager.cpp

#include "input_manager.hpp"
#include <iostream>

// コンストラクタ
InputManager::InputManager(GLFWwindow* window, Camera& camera)
    : m_window(window),
      m_camera(camera),
      m_lastX(0.0f),
      m_lastY(0.0f),
      m_firstMouse(true)
{
    // ここで glfwSetWindowUserPointer と glfwSetCursorPosCallback は設定しない！
    // Application クラスがこれらを担当する。

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // マウスカーソル非表示はここで設定
    resetMouseState(); // 初期化時にマウス位置をリセット
}

// 入力処理 (Escキーのみ)
void InputManager::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
    // WASDの処理はApplicationに残す
}

// マウス状態をリセットする
void InputManager::resetMouseState() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    m_lastX = static_cast<float>(width) / 2.0f;
    m_lastY = static_cast<float>(height) / 2.0f;
    m_firstMouse = true;

    glfwSetCursorPos(m_window, m_lastX, m_lastY);
}

// Applicationから呼ばれるマウス移動処理
void InputManager::processMouseMovement(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastX;
    float yoffset = m_lastY - ypos; // Y軸は下方向が正なので反転させる

    m_lastX = xpos;
    m_lastY = ypos;

    m_camera.processMouseMovement(xoffset, yoffset);
}