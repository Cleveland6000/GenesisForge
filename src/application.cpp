#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>

#include "noise/perlin_noise_2d.hpp" // Worldクラスに移動
#include "chunk_mesh_generator.hpp" // Worldクラスに移動
#include "chunk_renderer.hpp"     // Worldクラスに移動

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

Application::Application()
{
    std::cout << "--- Application constructor called. ---\n";
}

Application::~Application()
{
    std::cout << "--- Application destructor called. ---\n";
    glfwTerminate();
}

bool Application::initialize()
{
    std::cout << "Application::initialize started.\n";
    if (!setupDependenciesAndLoadResources())
    {
        std::cerr << "Application: Failed to setup dependencies and load resources. Exiting.\n";
        return false;
    }
    std::cout << "Dependencies and resources setup successfully.\n";

    setupCallbacks();
    std::cout << "Callbacks setup successfully.\n";

    if (!m_fontLoader || !m_fontData)
    {
        std::cerr << "Error: FontLoader or FontData is null in Application::initialize.\n";
        return false;
    }

    // フォントデータのロード
    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }
    std::cout << "FontData loaded by Application.\n";

    // レンダラーの初期化
    if (m_renderer && !m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }
    std::cout << "Renderer initialized successfully.\n";

    // Worldの初期チャンクをロードします (カメラの初期位置に基づいて)
    // カメラの初期位置は(0,0,0)なので、ワールドの中心にいると仮定します。
    if (m_world) {
        m_world->updateChunks(m_camera->getPosition());
        std::cout << "Initial chunks loaded by World.\n";
    } else {
        std::cerr << "Error: World object is null in Application::initialize.\n";
        return false;
    }

    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &initialWidth, &initialHeight);
    updateProjectionMatrix(initialWidth, initialHeight);
    std::cout << "Application initialized successfully.\n";
    return true;
}

void Application::run()
{
    std::cout << "Application::run started.\n";
    while (!m_windowContext->shouldClose())
    {
        processInput();
        update();
        render();
        m_windowContext->swapBuffers();
        m_windowContext->pollEvents();
    }
    std::cout << "Application::run finished.\n";
}

void Application::setupCallbacks()
{
    // フレームバッファサイズ変更コールバック
    m_windowContext->setFramebufferSizeCallback([this](int w, int h)
    {
        this->updateProjectionMatrix(w, h);
    });

    // カーソル位置コールバック
    m_windowContext->setCursorPosCallback([this](double xpos, double ypos)
    {
        if (this->m_inputManager) {
            this->m_inputManager->processMouseMovement(xpos, ypos);
        }
    });

    // フルスクリーンマネージャーのコールバックもここで設定
    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
    {
        this->updateProjectionMatrix(w, h);
    });

    m_fullscreenManager->setMouseResetCallback([this]()
    {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        }
    });
}

bool Application::setupDependenciesAndLoadResources()
{
    std::cout << "Application::setupDependenciesAndLoadResources started.\n";

    // WindowContextの初期化
    m_windowContext = std::make_unique<WindowContext>("Hello OpenGL Cubes", SCR_WIDTH, SCR_HEIGHT);
    if (!m_windowContext || !m_windowContext->initialize())
    {
        std::cerr << "Application: Failed to create or initialize WindowContext.\n";
        return false;
    }
    std::cout << "WindowContext created and initialized.\n";

    // Cameraの作成
    m_camera = std::make_unique<Camera>(glm::vec3(0.0f)); // カメラの初期位置をチャンクの中心付近に調整するかもしれません
    if (!m_camera) { std::cerr << "Application: Failed to create Camera.\n"; return false; }
    std::cout << "Camera created.\n";

    // Timerの作成
    m_timer = std::make_unique<Timer>();
    if (!m_timer) { std::cerr << "Application: Failed to create Timer.\n"; return false; }
    std::cout << "Timer created.\n";

    // InputManagerの作成
    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager) { std::cerr << "Application: Failed to create InputManager.\n"; return false; }
    m_inputManager->setWindow(m_windowContext->getWindow()); // InputManagerにウィンドウを設定
    std::cout << "InputManager created.\n";

    // FullscreenManagerの作成
    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager) { std::cerr << "Application: Failed to create FullscreenManager.\n"; return false; }
    std::cout << "FullscreenManager created.\n";

    // FontLoaderの作成
    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader) { std::cerr << "Application: Failed to create FontLoader.\n"; return false; }
    std::cout << "FontLoader created.\n";

    // FontDataの作成
    m_fontData = std::make_unique<FontData>();
    if (!m_fontData) { std::cerr << "Application: Failed to create FontData unique_ptr.\n"; return false; }
    std::cout << "FontData unique_ptr created.\n";

    // Rendererの作成
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer) { std::cerr << "Application: Failed to create Renderer.\n"; return false; }
    std::cout << "Renderer created.\n";
    
    // Worldの作成
    m_world = std::make_unique<World>(CHUNK_GRID_SIZE, CHUNK_RENDER_DISTANCE, m_cubeSpacing);
    if (!m_world) { std::cerr << "Application: Failed to create World.\n"; return false; }
    std::cout << "World created.\n";

    // 初期状態でフルスクリーンにする場合 (オプション)
    // m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());

    std::cout << "Application::setupDependenciesAndLoadResources finished successfully.\n";
    return true;
}

void Application::processInput()
{
    // F11キーでフルスクリーン切り替え
    static bool f11_last = false;
    if (glfwGetKey(m_windowContext->getWindow(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());
        f11_last = true;
    }
    else
        f11_last = false;

    // カメラ入力処理
    if (m_inputManager)
        m_inputManager->processInput(); // マウス入力など
    
    // キーボードによるカメラ移動
    auto key = [this](int k) { return glfwGetKey(m_windowContext->getWindow(), k) == GLFW_PRESS; };
    m_camera->processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A), key(GLFW_KEY_D), m_timer->getDeltaTime());
    m_camera->processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL), m_timer->getDeltaTime());
}

void Application::update()
{
    m_timer->tick(); // タイマーを更新

    // チャンクのロード/アンロードを更新 (プレイヤーの位置に基づいて)
    if (m_world) {
        m_world->updateChunks(m_camera->getPosition());
    }

    updateFpsAndPositionStrings(); // FPSと位置文字列を更新
}

void Application::updateFpsAndPositionStrings()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    frameCount++;

    // 1秒ごとにFPSを更新
    if (m_timer->getDeltaTime() > 0.0 && m_timer->getTotalTime() - lastFPSTime >= 1.0)
    {
        double fps = frameCount / (m_timer->getTotalTime() - lastFPSTime);
        m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));
        frameCount = 0;
        lastFPSTime = m_timer->getTotalTime();
    }

    // カメラの位置を更新
    glm::vec3 pos = m_camera->getPosition();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "Pos: X: " << pos.x << " Y: " << pos.y << " Z: " << pos.z;
    m_positionString = ss.str();
}

void Application::render()
{
    if (!m_renderer)
    {
        std::cerr << "Renderer is not initialized!\n";
        return;
    }

    // フレーム開始
    m_renderer->beginFrame(glm::vec4(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A));

    glm::mat4 view = m_camera->getViewMatrix();

    // ワールド内の全てのレンダリング可能なチャンクをレンダリング
    if (m_world) {
        const auto& renderableChunks = m_world->getRenderableChunks();
        for (const auto& pair : renderableChunks) {
            const glm::ivec3& chunkCoord = pair.first;
            const ChunkRenderData& renderData = pair.second;

            // チャンクのワールド座標を計算します
            // 各チャンクは m_chunkSize * m_cubeSpacing の大きさを持つと仮定
            glm::vec3 chunkWorldPosition = glm::vec3(
                static_cast<float>(chunkCoord.x * CHUNK_GRID_SIZE * m_cubeSpacing),
                static_cast<float>(chunkCoord.y * CHUNK_GRID_SIZE * m_cubeSpacing),
                static_cast<float>(chunkCoord.z * CHUNK_GRID_SIZE * m_cubeSpacing)
            );
            
            // Renderer::renderScene関数がチャンクのワールド位置を受け入れるように更新する必要があるかもしれません
            m_renderer->renderScene(m_projectionMatrix, view, renderData, chunkWorldPosition);
        }
    }

    // オーバーレイHUDのレンダリング
    int w, h;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);

    // フレーム終了
    m_renderer->endFrame();
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 1000.0f); // 遠方クリップ面を拡張
}

