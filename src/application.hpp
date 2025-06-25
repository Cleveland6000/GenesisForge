#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>

// 他のインクルード
#include "camera.hpp"
#include "timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp" // FullscreenManagerをインクルード
#include "chunk.hpp"
#include "FontLoader.hpp" // FontDataのために残す
#include "renderer.hpp" // Rendererクラスをインクルード

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();

    // ウィンドウサイズ変更コールバックから呼ばれるpublicメソッド
    void updateProjectionMatrix(int width, int height);

private:
    static const float CLEAR_COLOR_R;
    static const float CLEAR_COLOR_G;
    static const float CLEAR_COLOR_B;
    static const float CLEAR_COLOR_A;

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;

    // OpenGL関連のメンバ変数はRendererに移動
    // GLuint m_VAO, m_VBO, m_EBO, m_shaderProgram; // これらはRendererに移動

    Camera m_camera;
    Timer m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    FullscreenManager m_fullscreenManager; // FullscreenManagerを追加
    FontLoader m_fontLoader; // FontDataをロードするために残す
    FontData m_fontData; // TextRendererはRendererが持つため、FontDataのみここに残す

    std::unique_ptr<Chunk> m_chunk;

    // 新しいRendererインスタンス
    std::unique_ptr<Renderer> m_renderer; 

    // FPSと座標表示のための文字列
    std::string m_fpsString;
    std::string m_positionString;

    // 定数
    static constexpr int SCR_WIDTH = 800;
    static constexpr int SCR_HEIGHT = 600;
    static constexpr int m_gridSize = 16;     // グリッドサイズをメンバ変数として定義
    static constexpr float m_cubeSpacing = 1.0f; // 立方体間の間隔（ボクセルのスケール）

    // プロジェクション行列（リサイズイベントで更新される）
    glm::mat4 m_projectionMatrix;

    void processInput();
    void update();
    void render();

    // コールバック関数 (staticにする必要がある)
    static void staticFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void staticMouseCallback(GLFWwindow* window, double xposIn, double yposIn);
};

#endif // APPLICATION_HPP