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
#include "chunk/chunk.hpp" // Chunkクラスはまだ必要です
#include "renderer.hpp"
#include "FontLoader.hpp"
#include "TextRenderer.hpp" // TextRendererも含まれるか確認
#include "chunk_mesh_generator.hpp"
#include "chunk_renderer.hpp"
#include "world.hpp" // 新しく追加するWorldクラス

class Application {
public:
    Application();
    ~Application();

    // アプリケーションを初期化します
    bool initialize();
    // アプリケーションのメインループを実行します
    void run();

private:
    // クリアカラーの定数
    static constexpr float CLEAR_COLOR_R = 0.0f;
    static constexpr float CLEAR_COLOR_G = 0.0f;
    static constexpr float CLEAR_COLOR_B = 0.0f;
    static constexpr float CLEAR_COLOR_A = 1.0f;

    // ワールドを構成する各チャンクのボクセルサイズ (例: 64x64x64)
    static constexpr int CHUNK_GRID_SIZE = 64;
    // プレイヤーを中心にロードされるチャンクの範囲 (例: 3は中心から±3チャンク)
    static constexpr int CHUNK_RENDER_DISTANCE = 3;

    std::unique_ptr<WindowContext> m_windowContext;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Timer> m_timer;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<FullscreenManager> m_fullscreenManager;
    std::unique_ptr<FontData> m_fontData;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<FontLoader> m_fontLoader;
    std::unique_ptr<World> m_world; // 新しいWorldオブジェクト

    glm::mat4 m_projectionMatrix;
    float m_cubeSpacing = 1.0f; // キューブ間の間隔

    std::string m_fpsString;      // 表示用のFPS文字列
    std::string m_positionString; // 表示用の位置文字列

    // ChunkRenderData m_testCubeRenderData; // 単一チャンクのレンダリングデータは不要に

    // コールバック関数を設定します
    void setupCallbacks();
    // 依存関係をセットアップし、リソースをロードします
    bool setupDependenciesAndLoadResources();
    // ユーザー入力を処理します
    void processInput();
    // ゲームの状態を更新します
    void update();
    // FPSと位置の表示文字列を更新します
    void updateFpsAndPositionStrings();
    // シーンをレンダリングします
    void render();
    // プロジェクション行列を更新します
    void updateProjectionMatrix(int width, int height);
};

#endif // APPLICATION_HPP

