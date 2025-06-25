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
#include "input_manager.hpp" // InputManagerの宣言のために必要
#include "chunk.hpp"        // Chunkクラスの宣言のために必要 (新しく追加)

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();

    // ウィンドウリサイズ時にProjection Matrixを更新するためのパブリックメソッド
    void updateProjectionMatrix(int width, int height);

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

    // 入力マネージャー
    std::unique_ptr<InputManager> m_inputManager;

    // Chunk オブジェクト (ボクセルデータとその管理を委譲)
    std::unique_ptr<Chunk> m_chunk; // 新しく追加

    // === グリッドのサイズと間隔 (Applicationが管理) ===
    int m_gridSize = 16;           // グリッドの各次元のサイズ
    float m_cubeSpacing = 1.0f;    // 立方体間の間隔 (立方体のサイズと一致させる)

    // 投影行列
    glm::mat4 m_projectionMatrix;

    // フォントとテキストレンダリング関連
    FontLoader m_fontLoader;
    FontData m_fontData; // ロードされたフォントデータ
    TextRenderer m_textRenderer;
    std::string m_fpsString = "FPS: 0";       // 表示するFPS文字列
    std::string m_positionString = "Pos: X: 0.0 Y: 0.0 Z: 0.0"; // 表示する座標文字列

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
};

#endif // APPLICATION_HPP
