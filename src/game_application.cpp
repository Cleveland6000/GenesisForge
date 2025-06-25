// src/game_application.cpp

#include "game_application.hpp"
#include <utility>
#include <iostream>

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
      m_app(nullptr),
      m_windowContext(nullptr) // ★WindowContextのunique_ptrを追加★
{
    std::cout << "--- GameApplication constructor called. ---\n";
}

// GameApplicationのデストラクタでglfwTerminate()を呼び出す
GameApplication::~GameApplication() {
    std::cout << "--- GameApplication destructor called. ---\n";
    glfwTerminate(); // アプリケーション終了時にGLFWを終了
}

// setupDependencies 関数: 各依存オブジェクトを生成し、Applicationに渡す
bool GameApplication::setupDependencies()
{
    std::cout << "GameApplication::setupDependencies started.\n";

    // ★WindowContextの生成と初期化★
    m_windowContext = std::make_unique<WindowContext>("Hello OpenGL Cubes", SCR_WIDTH, SCR_HEIGHT);
    if (!m_windowContext || !m_windowContext->initialize()) {
        std::cerr << "GameApplication: Failed to create or initialize WindowContext.\n";
        return false;
    }
    std::cout << "WindowContext created and initialized.\n";

    // Camera の生成
    m_camera = std::make_unique<Camera>(glm::vec3(0.0f));
    if (!m_camera) { std::cerr << "GameApplication: Failed to create Camera.\n"; return false; }
    std::cout << "Camera created.\n";

    // Timer の生成
    m_timer = std::make_unique<Timer>();
    if (!m_timer) { std::cerr << "GameApplication: Failed to create Timer.\n"; return false; }
    std::cout << "Timer created.\n";

    // InputManager の生成 (Cameraに依存)
    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager) { std::cerr << "GameApplication: Failed to create InputManager.\n"; return false; }
    std::cout << "InputManager created.\n";

    // FullscreenManager の生成
    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager) { std::cerr << "GameApplication: Failed to create FullscreenManager.\n"; return false; }
    std::cout << "FullscreenManager created.\n";

    // FontLoader の生成
    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader) { std::cerr << "GameApplication: Failed to create FontLoader.\n"; return false; }
    std::cout << "FontLoader created.\n";
    
    // FontData の生成 (データ構造のみ、テクスチャロードはまだ行わない)
    std::cout << "Attempting to create FontData unique_ptr.\n";
    m_fontData = std::make_unique<FontData>();
    if (!m_fontData) {
        std::cerr << "GameApplication: Failed to create FontData unique_ptr.\n";
        return false;
    }
    std::cout << "FontData unique_ptr created.\n";

    // Chunk の生成
    m_chunk = std::make_unique<Chunk>(CHUNK_GRID_SIZE, 0.3f);
    if (!m_chunk) { std::cerr << "GameApplication: Failed to create Chunk.\n"; return false; }
    std::cout << "Chunk created.\n";

    // Renderer の生成
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer) { std::cerr << "GameApplication: Failed to create Renderer.\n"; return false; }
    std::cout << "Renderer created.\n";

    // Application インスタンスの構築と依存オブジェクトの注入
    // WindowContext, FontLoader, FontData の所有権を Application に移動する
    m_app = std::make_unique<Application>(
        std::move(m_windowContext), // ★WindowContextをムーブ★
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
        std::cerr << "GameApplication: Failed to create core Application instance.\n";
        return false;
    }
    std::cout << "Core Application instance created.\n";

    std::cout << "GameApplication::setupDependencies finished successfully.\n";
    return true;
}

// run 関数: アプリケーションのメイン実行ループ
int GameApplication::run()
{
    // 依存オブジェクトとApplicationインスタンスをセットアップ
    if (!setupDependencies()) {
        std::cerr << "GameApplication: Failed to setup dependencies. Exiting.\n";
        return -1;
    }
    std::cout << "GameApplication: Dependencies setup completed. Proceeding to Application initialize.\n";

    // Applicationクラスの初期化と実行
    if (!m_app->initialize()) {
        std::cerr << "GameApplication: Core Application initialization failed. Exiting.\n";
        return -1;
    }
    std::cout << "GameApplication: Application initialized. Starting run loop.\n";
    
    m_app->run(); // Applicationのメインループを実行

    std::cout << "GameApplication: Application run loop finished.\n";
    return 0;
}