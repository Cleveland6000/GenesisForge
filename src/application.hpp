// src/application.hpp (既存のコードに以下の変更を追加)

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>
#include <string>
#include <glm/glm.hpp>

// 既存のインクルード群
#include "window_context.hpp"
#include "camera.hpp"
#include "time/timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "chunk/chunk.hpp"
#include "renderer.hpp"
#include "FontLoader.hpp"
#include "TextRenderer.hpp" // TextRenderer も必要に応じてインクルードされているか確認
#include "chunk_mesh_generator.hpp" // !!!ここを追加!!!
#include "chunk_renderer.hpp"     // !!!ここを追加!!!

struct FontData; // 前方宣言 (FontLoader.hpp で定義されているはず)

class Application
{
public:
    Application(std::unique_ptr<WindowContext> windowContext,
                std::unique_ptr<Camera> camera,
                std::unique_ptr<Timer> timer,
                std::unique_ptr<InputManager> inputManager,
                std::unique_ptr<FullscreenManager> fullscreenManager,
                std::unique_ptr<Chunk> chunk,
                std::unique_ptr<Renderer> renderer,
                std::unique_ptr<FontData> fontData,
                std::unique_ptr<FontLoader> fontLoader);
    ~Application();

    bool initialize();
    void run();

private:
    void setupCallbacks();
    bool initializeManagersAndLoadResources();
    void processInput();
    void update();
    void updateFpsAndPositionStrings();
    void render();
    void updateProjectionMatrix(int width, int height);

    static const float CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A;

    std::unique_ptr<WindowContext> m_windowContext;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<Chunk> m_chunk;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<FontLoader> m_fontLoader;

    glm::mat4 m_projectionMatrix;
    float m_cubeSpacing = 1.0f; // 必要に応じて調整
    std::string m_fpsString;
    std::string m_positionString;

    // !!!ここが変更点!!! テスト用のChunkRenderDataをメンバ変数として追加
    ChunkRenderData m_testCubeRenderData; 
};

#endif // APPLICATION_HPP