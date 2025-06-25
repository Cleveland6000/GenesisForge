// src/application.hpp

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

// 依存オブジェクトのヘッダー
#include "camera.hpp"
#include "timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "chunk.hpp"
#include "FontLoader.hpp" // FontData のために必要
#include "renderer.hpp"

// ★WindowContextのヘッダーを追加★
#include "window_context.hpp"

class Application
{
public:
    // ★WindowContextのunique_ptrを受け取るように変更★
    Application(std::unique_ptr<WindowContext> windowContext, // WindowContextをDIで受け取る
                std::unique_ptr<Camera> camera,
                std::unique_ptr<Timer> timer,
                std::unique_ptr<InputManager> inputManager,
                std::unique_ptr<FullscreenManager> fullscreenManager,
                std::unique_ptr<Chunk> chunk,
                std::unique_ptr<Renderer> renderer,
                std::unique_ptr<FontData> fontData,
                std::unique_ptr<FontLoader> fontLoader);

    ~Application();

    bool initialize(); // ここでは、OpenGLコンテキストは既に作成されている前提

    void run();

private:
    // ★GLFWwindow* m_window; の代わりに WindowContext のunique_ptrを持つ★
    std::unique_ptr<WindowContext> m_windowContext;

    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<FontLoader> m_fontLoader;
    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;

    // ゲームの定数
    static const float CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A;

    glm::mat4 m_projectionMatrix;
    std::string m_fpsString;
    std::string m_positionString;

    float m_cubeSpacing = 1.0f;
    // プライベートメソッド
    // ★initGLFW() と createWindowAndContext() は削除★
    void setupCallbacks(); // コールバックの登録は WindowContext 経由で行う
    bool initializeManagersAndLoadResources();
    void processInput();
    void update();
    void updateFpsAndPositionStrings();
    void render();
    // 内部で使用するプロジェクション行列の更新
    void updateProjectionMatrix(int width, int height);

    // ★GLFWコールバックの静的ラッパー関数は不要になる（WindowContextが持つため）★
    // static void staticFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    // static void staticMouseCallback(GLFWwindow* window, double xpos, double ypos);
};

#endif // APPLICATION_HPP