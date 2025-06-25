#ifndef FULLSCREEN_MANAGER_HPP
#define FULLSCREEN_MANAGER_HPP

#include <GLFW/glfw3.h>
#include <iostream> // std::cerr のために必要

// ウィンドウサイズ変更時に呼ばれるコールバック関数の型を定義
// width と height を引数として受け取る
using WindowSizeChangeCallback = void (*)(int, int);

// マウス状態リセット時に呼ばれるコールバック関数の型を定義
using MouseResetCallback = void (*)();

class FullscreenManager
{
public:
    FullscreenManager();

    // フルスクリーン切り替え関数
    // 依存する Application へのポインタは不要になる
    void toggleFullscreen(GLFWwindow *window);

    // ウィンドウサイズ変更時のコールバック関数を設定するメソッド
    void setWindowSizeChangeCallback(WindowSizeChangeCallback callback) {
        m_windowSizeChangeCallback = callback;
    }

    // マウス状態リセット時のコールバック関数を設定するメソッド
    void setMouseResetCallback(MouseResetCallback callback) {
        m_mouseResetCallback = callback;
    }

    bool isFullscreen() const { return m_isFullscreen; } // 現在の状態を取得

private:
    bool m_isFullscreen;
    int m_windowedX, m_windowedY;
    int m_windowedWidth, m_windowedHeight;

    // 登録されたコールバック関数ポインタ
    WindowSizeChangeCallback m_windowSizeChangeCallback;
    MouseResetCallback m_mouseResetCallback;
};

#endif // FULLSCREEN_MANAGER_HPP