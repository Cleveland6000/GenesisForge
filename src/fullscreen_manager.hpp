#ifndef FULLSCREEN_MANAGER_HPP
#define FULLSCREEN_MANAGER_HPP

#include <GLFW/glfw3.h>
#include <functional> // std::function のために必要

// コールバック型の定義を std::function に変更
using WindowSizeChangeCallback = std::function<void(int, int)>;
using MouseResetCallback = std::function<void()>;

class FullscreenManager
{
public:
    FullscreenManager();

    void toggleFullscreen(GLFWwindow *window);

    // コールバックセッターの型を std::function に変更
    void setWindowSizeChangeCallback(WindowSizeChangeCallback cb) { m_windowSizeChangeCallback = cb; }
    void setMouseResetCallback(MouseResetCallback cb) { m_mouseResetCallback = cb; }

private:
    bool m_isFullscreen;
    int m_windowedPosX, m_windowedPosY;
    int m_windowedWidth, m_windowedHeight;

    WindowSizeChangeCallback m_windowSizeChangeCallback;
    MouseResetCallback m_mouseResetCallback;
};

#endif // FULLSCREEN_MANAGER_HPP