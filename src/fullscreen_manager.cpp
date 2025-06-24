#include "fullscreen_manager.hpp"
#include "application.hpp" // ApplicationクラスのupdateProjectionMatrixを呼び出すためにインクルード

FullscreenManager::FullscreenManager()
    : m_isFullscreen(true), m_windowedX(0), m_windowedY(0), m_windowedWidth(0), m_windowedHeight(0)
{
}

void FullscreenManager::toggleFullscreen(GLFWwindow *window)
{
    if (m_isFullscreen)
    {
        // フルスクリーン -> ウィンドウモード
        glfwSetWindowMonitor(window, NULL,
                             m_windowedX, m_windowedY,
                             m_windowedWidth, m_windowedHeight,
                             GLFW_DONT_CARE);

        m_isFullscreen = false;
    }
    else
    {
        // ウィンドウモード -> フルスクリーン
        glfwGetWindowPos(window, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(window, &m_windowedWidth, &m_windowedHeight);

        GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
        if (!primaryMonitor)
        {
            std::cerr << "Failed to get primary monitor.\n";
            return;
        }

        const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);
        if (!mode)
        {
            std::cerr << "Failed to get video mode for primary monitor.\n";
            return;
        }

        glfwSetWindowMonitor(window, primaryMonitor,
                             0, 0,
                             mode->width, mode->height,
                             mode->refreshRate);

        m_isFullscreen = true;
    }

    int actualWidth, actualHeight;
    glfwGetFramebufferSize(window, &actualWidth, &actualHeight);

    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->updateProjectionMatrix(actualWidth, actualHeight);
        app->resetMouseState(); // 【追加】フルスクリーン切り替え後にマウス状態をリセット
    }
    glViewport(0, 0, actualWidth, actualHeight);
}