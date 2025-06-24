#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::perspective, glm::lookAt
#include <glm/gtc/type_ptr.hpp>         // glm::value_ptr

#include "camera.hpp"
#include "timer.hpp"
#include "fullscreen_manager.hpp"
#include "FontLoader.hpp"   // FontLoaderの宣言のために必要
#include "TextRenderer.hpp" // TextRendererの宣言のために必要

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();

    // ウィンドウリサイズ時にProjection Matrixを更新するためのパブリックメソッド
    void updateProjectionMatrix(int width, int height);
    // マウス状態をリセットする関数
    void resetMouseState();

private:
    // GLFWwindowを管理するためのunique_ptr
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;

    // OpenGLオブジェクト
    unsigned int m_VAO, m_VBO, m_EBO;
    unsigned int m_shaderProgram;

    // カメラオブジェクト
    Camera m_camera;

    // タイマーオブジェクト
    Timer m_timer;

    // フルスクリーンマネージャー
    FullscreenManager m_fullscreenManager;

    // キューブの位置
    std::vector<glm::vec3> m_cubePositions;

    // マウス入力関連
    float m_lastX = 400.0f; // ウィンドウの中心を初期値とする (仮)
    float m_lastY = 300.0f; // ウィンドウの中心を初期値とする (仮)
    bool m_firstMouse = true;

    // 投影行列
    glm::mat4 m_projectionMatrix;

    // フォントとテキストレンダリング関連
    FontLoader m_fontLoader;
    FontData m_fontData; // ロードされたフォントデータ
    TextRenderer m_textRenderer;
    std::string m_fpsString = "FPS: 0"; // 表示するFPS文字列

    // 定数
    static const float CLEAR_COLOR_R;
    static const float CLEAR_COLOR_G;
    static const float CLEAR_COLOR_B;
    static const float CLEAR_COLOR_A;
    static const unsigned int SCR_WIDTH = 800;
    static const unsigned int SCR_HEIGHT = 600;

    // プライベートヘルパー関数
    void processInput();
    void update();
    void render();

    // 静的コールバック関数 (CスタイルコールバックとしてApplicationインスタンスにディスパッチするため)
    static void staticFramebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void staticMouseCallback(GLFWwindow *window, double xposIn, double yposIn);
    // static void staticScrollCallback(GLFWwindow *window, double xoffset, double yoffset); // 削除
};

#endif // APPLICATION_HPP