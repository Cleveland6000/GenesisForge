#include "fullscreen_manager.hpp"
#include "application.hpp" // ApplicationクラスのupdateProjectionMatrixを呼び出すためにインクルード

FullscreenManager::FullscreenManager()
    : m_isFullscreen(false), m_windowedX(0), m_windowedY(0), m_windowedWidth(0), m_windowedHeight(0)
{
}

void FullscreenManager::toggleFullscreen(GLFWwindow *window, int currentWidth, int currentHeight)
{
    if (m_isFullscreen)
    {
        // フルスクリーン -> ウィンドウモード
        glfwSetWindowMonitor(window, NULL, // NULL を渡すとウィンドウモードになる
                             m_windowedX, m_windowedY,
                             m_windowedWidth, m_windowedHeight,
                             GLFW_DONT_CARE); // リフレッシュレートは気にしない

        m_isFullscreen = false;
    }
    else
    {
        // ウィンドウモード -> フルスクリーン
        // 現在のウィンドウの位置とサイズを保存
        glfwGetWindowPos(window, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(window, &m_windowedWidth, &m_windowedHeight);

        // プライマリモニターを取得
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
                             0, 0,                      // フルスクリーンなので位置は(0,0)
                             mode->width, mode->height, // モニターの解像度
                             mode->refreshRate);        // モニターのリフレッシュレート

        m_isFullscreen = true;
    }

    // モード切り替え後、フレームバッファサイズが変わる可能性があるので、
    // ここでApplication::updateProjectionMatrixを呼び出すためにApplicationインスタンスを取得し、
    // そのメソッドを呼び出す。
    // または、FullscreenManagerがイベントを発行し、Applicationがそれを受け取るような設計も考えられます。
    // 今回は簡略化のため、Applicationクラスに依存する形で実装します。
    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->updateProjectionMatrix(currentWidth, currentHeight); // ウィンドウサイズは変わる可能性があるが、現在の値でプロジェクションを更新
    }
    glViewport(0, 0, currentWidth, currentHeight); // Viewportも更新
}