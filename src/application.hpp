#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>

// 依存するクラスのヘッダー
#include "camera.hpp"
#include "timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "chunk.hpp"
#include "FontLoader.hpp" // FontLoaderは引き続き必要
#include "renderer.hpp"

class Application // このクラスが主要なゲームロジックを担う
{
public:
    // コンストラクタ引数からFontLoaderを削除し、FontDataは参照で受け取る
    Application(std::unique_ptr<Camera> camera,
                std::unique_ptr<Timer> timer,
                std::unique_ptr<InputManager> inputManager,
                std::unique_ptr<FullscreenManager> fullscreenManager,
                std::unique_ptr<Chunk> chunk,
                std::unique_ptr<Renderer> renderer,
                std::unique_ptr<FontData> fontData,      // FontDataもunique_ptrで受け取る
                std::unique_ptr<FontLoader> fontLoader); // FontLoaderもunique_ptrで受け取る

    ~Application();

    bool initialize();
    void run(); // これはAppBuilderのrunとは異なる、Application自体のメインループ
    void updateProjectionMatrix(int width, int height);

private:
    static const float CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A;
    static constexpr int SCR_WIDTH = 800, SCR_HEIGHT = 600, m_gridSize = 16;
    static constexpr float m_cubeSpacing = 1.0f;

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> m_window;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    // std::unique_ptr<FontLoader> m_fontLoader; // GameApplicationが所有する
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<FontLoader> m_fontLoader; // FontLoaderもメンバーとして持つ
    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;

    std::string m_fpsString, m_positionString;
    glm::mat4 m_projectionMatrix;

    bool initGLFW();
    bool createWindowAndContext();
    void setupCallbacks();
    bool initializeManagersAndLoadResources();

    void processInput();
    void update();
    void render();
    void updateFpsAndPositionStrings(); // メソッド名を修正 (タイポの可能性)

    static void staticFramebufferSizeCallback(GLFWwindow *, int, int);
    static void staticMouseCallback(GLFWwindow *, double, double);
};

#endif // APPLICATION_HPP