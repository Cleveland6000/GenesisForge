#ifndef FULLSCREEN_MANAGER_HPP
#define FULLSCREEN_MANAGER_HPP

#include <GLFW/glfw3.h>
#include <iostream>

using WindowSizeChangeCallback = void (*)(int, int);
using MouseResetCallback = void (*)();

class FullscreenManager {
public:
    FullscreenManager();

    void toggleFullscreen(GLFWwindow *window);

    void setWindowSizeChangeCallback(WindowSizeChangeCallback cb) { m_windowSizeChangeCallback = cb; }
    void setMouseResetCallback(MouseResetCallback cb) { m_mouseResetCallback = cb; }

    bool isFullscreen() const { return m_isFullscreen; }

private:
    bool m_isFullscreen;
    int m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight;
    WindowSizeChangeCallback m_windowSizeChangeCallback;
    MouseResetCallback m_mouseResetCallback;
};

#endif // FULLSCREEN_MANAGER_HPP
