#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <GLFW/glfw3.h>
#include "camera.hpp" // Cameraクラスが必要

class InputManager
{
public:
    // GLFWwindow* をコンストラクタから削除し、setWindowメソッドを追加
    InputManager(Camera& camera); // Cameraは参照で受け取る
    void setWindow(GLFWwindow* window); // 後からウィンドウを設定するメソッド

    void processInput(); // キーボード入力処理
    void processMouseMovement(double xpos, double ypos); // マウス移動処理
    void resetMouseState(); // マウスの状態をリセット

private:
    GLFWwindow* m_window; // ここでポインタを保持
    Camera& m_camera; // Cameraへの参照
    
    bool m_firstMouse;
    float m_lastX;
    float m_lastY;
};

#endif // INPUT_MANAGER_HPP