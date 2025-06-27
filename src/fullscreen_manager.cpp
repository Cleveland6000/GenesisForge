#include "fullscreen_manager.hpp"
#include <iostream> // std::cerr のために追加

// 以前のコメントにあったように、Applicationクラスをインクルードする必要はありません。

FullscreenManager::FullscreenManager()
    : m_isFullscreen(true),
      m_windowedPosX(0), // m_windowedX を m_windowedPosX に修正
      m_windowedPosY(0), // m_windowedY を m_windowedPosY に修正
      m_windowedWidth(0),
      m_windowedHeight(0),
      m_windowSizeChangeCallback(nullptr), // std::function は nullptr で初期化可能
      m_mouseResetCallback(nullptr)
{
}

void FullscreenManager::toggleFullscreen(GLFWwindow* window)
{
    if (m_isFullscreen)
    {
        // フルスクリーンからウィンドウモードへ
        glfwSetWindowMonitor(window, NULL,
                             m_windowedPosX, m_windowedPosY, // m_windowedX, m_windowedY を m_windowedPosX, m_windowedPosY に修正
                             m_windowedWidth, m_windowedHeight, 0);
    }
    else
    {
        // ウィンドウモードからフルスクリーンへ
        // 現在のウィンドウの位置とサイズを保存
        glfwGetWindowPos(window, &m_windowedPosX, &m_windowedPosY);     // m_windowedX を m_windowedPosX に修正
        glfwGetWindowSize(window, &m_windowedWidth, &m_windowedHeight); // m_windowedY を m_windowedPosY に修正

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        if (!primaryMonitor)
        {
            std::cerr << "Failed to get primary monitor.\n"; // std::cerr のために <iostream> をインクルード
            return;
        }

        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
        if (!mode)
        {
            std::cerr << "Failed to get video mode for primary monitor.\n"; // std::cerr のために <iostream> をインクルード
            return;
        }

        glfwSetWindowMonitor(window, primaryMonitor, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
    }

    m_isFullscreen = !m_isFullscreen;

    // コールバックが設定されていれば呼び出す
    if (m_windowSizeChangeCallback)
    {
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        m_windowSizeChangeCallback(currentWidth, currentHeight);
    }
    if (m_mouseResetCallback)
    {
        m_mouseResetCallback();
    }
}