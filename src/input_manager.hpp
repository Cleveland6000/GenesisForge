// input_manager.hpp

#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <GLFW/glfw3.h>
#include "camera.hpp" // Cameraクラスが必要

class InputManager {
public:
    // コンストラクタでウィンドウとカメラへの参照を受け取る
    InputManager(GLFWwindow* window, Camera& camera);

    // 入力処理を行うメイン関数
    void processInput();

    // マウスカーソルをリセットする関数 (フルスクリーン切り替え時などに使用)
    void resetMouseState();

    // Applicationから呼ばれるためのマウスコールバックの実装
    void processMouseMovement(double xposIn, double yposIn); // これをpublicにする

private:
    GLFWwindow* m_window;
    Camera& m_camera; // カメラへの参照

    // マウス入力関連の内部状態
    float m_lastX;
    float m_lastY;
    bool m_firstMouse;

    // static callbackはApplicationが担当するため、ここからは削除
    // static void staticMouseCallback(GLFWwindow* window, double xposIn, double yposIn);

    // プライベートなマウスコールバックの実装 (名前を変更し、publicなラッパーから呼ばれる)
    // void mouseCallback(double xposIn, double yposIn); // これが public processMouseMovement に変更
};

#endif // INPUT_MANAGER_HPP