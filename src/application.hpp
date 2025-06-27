#ifndef APPLICATION_HPP
#define APPLICATION_HPP
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <iostream>
#include "window_context.hpp"
#include "camera.hpp"
#include "time/timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
// #include "chunk/chunk.hpp" // ChunkManagerがChunkを管理するので不要
#include "renderer.hpp"
#include "FontLoader.hpp"
#include "TextRenderer.hpp"
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "chunk_manager.hpp" // 新しく追加

class Application
{
public:
    Application();
    ~Application();
    bool initialize();
    void run();

private:
    static constexpr float CLEAR_COLOR_R = 0.0f;
    static constexpr float CLEAR_COLOR_G = 0.0f;
    static constexpr float CLEAR_COLOR_B = 0.0f;
    static constexpr float CLEAR_COLOR_A = 1.0f;

    std::unique_ptr<WindowContext> m_windowContext;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontData> m_fontData;
    // std::unique_ptr<Chunk> m_chunk; // チャンク管理クラスに移行
    std::unique_ptr<ChunkManager> m_chunkManager; // 新しく追加
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<FontLoader> m_fontLoader;

    glm::mat4 m_projectionMatrix;
    // float m_cubeSpacing = 1.0f; // ChunkManagerまたはRendererで扱う
    std::string m_fpsString;
    std::string m_positionString;
    // ChunkRenderData m_testCubeRenderData; // チャンク管理クラスに移行

    static constexpr int CHUNK_GRID_SIZE = 16; // チャンクのサイズ
    static constexpr float NOISE_SCALE = 0.05f; // Perlinノイズのスケール
    static constexpr int RENDER_DISTANCE_CHUNKS = 3; // プレイヤーからのチャンクの描画距離（チャンク単位）

    void setupCallbacks();
    bool setupDependenciesAndLoadResources();
    void processInput();
    void update();
    void updateFpsAndPositionStrings();
    void render();
    void updateProjectionMatrix(int width, int height);
};
#endif // APPLICATION_HPP