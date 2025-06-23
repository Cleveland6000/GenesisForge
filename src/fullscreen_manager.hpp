#ifndef FULLSCREEN_MANAGER_HPP
#define FULLSCREEN_MANAGER_HPP

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>

class FullscreenManager
{
public:
    FullscreenManager();

    // フルスクリーン状態をトグルする関数
    // currentWidth, currentHeight の引数を削除
    void toggleFullscreen(GLFWwindow *window);

    // フルスクリーン状態を取得
    bool isFullscreen() const { return m_isFullscreen; }

private:
    bool m_isFullscreen;
    int m_windowedX, m_windowedY;
    int m_windowedWidth, m_windowedHeight;
};

#endif // FULLSCREEN_MANAGER_HPP