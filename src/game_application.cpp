#include "game_application.hpp"
#include <utility> // std::move のために必要

// GameApplicationのコンストラクタはnullptr初期化のみ
GameApplication::GameApplication()
    : m_camera(nullptr),
      m_timer(nullptr),
      m_inputManager(nullptr),
      m_fullscreenManager(nullptr),
      m_fontLoader(nullptr),
      m_fontData(nullptr),
      m_chunk(nullptr),
      m_renderer(nullptr),
      m_app(nullptr)
{
    // 必要な場合のみログ出力
}

// setupDependencies 関数: 各依存オブジェクトを生成し、Applicationに渡す
bool GameApplication::setupDependencies()
{
    // 依存オブジェクト生成失敗時のみエラー出力
    m_camera = std::make_unique<Camera>(glm::vec3(0.0f));
    if (!m_camera) { std::cerr << "[GameApplication] Failed to create Camera.\n"; return false; }

    m_timer = std::make_unique<Timer>();
    if (!m_timer) { std::cerr << "[GameApplication] Failed to create Timer.\n"; return false; }

    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager) { std::cerr << "[GameApplication] Failed to create InputManager.\n"; return false; }

    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager) { std::cerr << "[GameApplication] Failed to create FullscreenManager.\n"; return false; }

    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader) { std::cerr << "[GameApplication] Failed to create FontLoader.\n"; return false; }
    
    m_fontData = std::make_unique<FontData>();
    if (!m_fontData) {
        std::cerr << "[GameApplication] Failed to create FontData unique_ptr.\n";
        return false;
    }

    m_chunk = std::make_unique<Chunk>(CHUNK_GRID_SIZE, 0.3f);
    if (!m_chunk) { std::cerr << "[GameApplication] Failed to create Chunk.\n"; return false; }

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer) { std::cerr << "[GameApplication] Failed to create Renderer.\n"; return false; }

    m_app = std::make_unique<Application>(
        std::move(m_camera),
        std::move(m_timer),
        std::move(m_inputManager),
        std::move(m_fullscreenManager),
        std::move(m_chunk),
        std::move(m_renderer),
        std::move(m_fontData),
        std::move(m_fontLoader)
    );

    if (!m_app) {
        std::cerr << "[GameApplication] Failed to create core Application instance.\n";
        return false;
    }

    return true;
}

// run 関数: アプリケーションのメイン実行ループ
int GameApplication::run()
{
    // 初期化失敗時のみエラー出力
    if (!m_app->initialize()) {
        std::cerr << "[GameApplication] Core Application initialization failed. Exiting.\n";
        return -1;
    }
    m_app->run();
    return 0;
}
