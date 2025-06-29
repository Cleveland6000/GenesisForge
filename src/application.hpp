#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate のために必要
#include <array> // std::array のために必要

#include "window_context.hpp"
#include "camera.hpp"
#include "time/timer.hpp"
#include "input_manager.hpp"
#include "fullscreen_manager.hpp"
#include "renderer.hpp" // Renderer と ChunkRenderData のために必要
#include "FontLoader.hpp"
#include "TextRenderer.hpp"
// ChunkMeshGenerator, ChunkRenderer は直接Applicationで使用しないが、
// ChunkManagerが依存しているため、関連性が間接的にある
#include "chunk_manager.hpp" 

// 平面を表す構造体（Ax + By + Cz + D = 0）
struct Plane {
    glm::vec3 normal;
    float distance; // 原点からの距離
};

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
    std::unique_ptr<ChunkManager> m_chunkManager;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<FontLoader> m_fontLoader;

    glm::mat4 m_projectionMatrix;
    std::string m_fpsString;
    std::string m_positionString;

    // 定数定義 (あなたの既存のコードから移動)
    static constexpr int CHUNK_GRID_SIZE = 16;      // チャンクのサイズ
    static constexpr float NOISE_SCALE = 0.006f;    // パーリンノイズの全体的なスケール（細かさ）
    static constexpr int RENDER_DISTANCE_CHUNKS = 8; // プレイヤーからのチャンクの描画距離（X/Z軸チャンク単位）
    static constexpr unsigned int WORLD_SEED = 12345; // ワールド生成のための固定シード
    static constexpr int WORLD_MAX_HEIGHT = 24;     // ワールド全体のボクセルの最大高さ
    static constexpr int GROUND_LEVEL = 0;         // 地表の基準となる高さ (WORLD_MAX_HEIGHT の中間など)
    static constexpr int TERRAIN_OCTAVES = 4;       // ノイズのオクターブ数
    static constexpr float TERRAIN_LACUNARITY = 2.0f; // 各オクターブの周波数の倍率
    static constexpr float TERRAIN_PERSISTENCE = 0.5f; // 各オクターブの振幅の減衰率

    // フラスタムカリング関連のメンバー
    std::array<Plane, 6> m_frustumPlanes; // 視錐台の6つの平面

    void setupCallbacks();
    bool setupDependenciesAndLoadResources();
    void processInput();
    void update();
    void updateFpsAndPositionStrings();
    void render();
    void updateProjectionMatrix(int width, int height);

    // フラスタムカリング関連のメソッド
    void extractFrustumPlanes(const glm::mat4& viewProjection); // 結合行列から平面を抽出
    bool isChunkInFrustum(const glm::ivec3& chunkCoord) const; // チャンクが視錐台内にあるかチェック
};

#endif // APPLICATION_HPP
