#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
// #include "noise/perlin_noise_2d.hpp" // ChunkManagerが使用するので不要
// #include "chunk_mesh_generator.hpp"   // ChunkManagerが使用するので不要
// #include "chunk_renderer.hpp"         // ChunkManagerが使用するので不要

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
    if (!setupDependenciesAndLoadResources())
    {
        std::cerr << "Application: Failed to setup dependencies and load resources. Exiting.\n";
        return false;
    }
    setupCallbacks();

    if (!m_fontLoader || !m_fontData)
    {
        std::cerr << "Error: FontLoader or FontData is null in Application::initialize.\n";
        return false;
    }

    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }
    std::cout << "FontData loaded by Application.\n";

    if (m_renderer && !m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }

    // 初期チャンクのロードをトリガー
    // これにより、ゲーム開始時にプレイヤーの初期位置に基づいてチャンクがロードされます
    if (m_chunkManager) {
        m_chunkManager->update(m_camera->getPosition());
    } else {
        std::cerr << "Error: ChunkManager is null in Application::initialize after setupDependenciesAndLoadResources.\n";
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
    while (!m_windowContext->shouldClose())
    {
        processInput();
        update();
        render();
        m_windowContext->swapBuffers();
        m_windowContext->pollEvents();
    }
}

void Application::setupCallbacks()
{
    m_windowContext->setFramebufferSizeCallback([this](int w, int h)
                                                 { this->updateProjectionMatrix(w, h); });
    m_windowContext->setCursorPosCallback([this](double xpos, double ypos)
                                           {
        if (this->m_inputManager) {
            this->m_inputManager->processMouseMovement(xpos, ypos);
        } });
}

bool Application::setupDependenciesAndLoadResources()
{
    std::cout << "Application::setupDependenciesAndLoadResources started.\n";

    m_windowContext = std::make_unique<WindowContext>("Hello OpenGL Cubes", SCR_WIDTH, SCR_HEIGHT);
    if (!m_windowContext || !m_windowContext->initialize())
    {
        std::cerr << "Application: Failed to create or initialize WindowContext.\n";
        return false;
    }
    std::cout << "WindowContext created and initialized.\n";

    m_camera = std::make_unique<Camera>(glm::vec3(0.0f));
    if (!m_camera)
    {
        std::cerr << "Application: Failed to create Camera.\n";
        return false;
    }
    std::cout << "Camera created.\n";

    m_timer = std::make_unique<Timer>();
    if (!m_timer)
    {
        std::cerr << "Application: Failed to create Timer.\n";
        return false;
    }
    std::cout << "Timer created.\n";

    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager)
    {
        std::cerr << "Application: Failed to create InputManager.\n";
        return false;
    }
    std::cout << "InputManager created.\n";

    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager)
    {
        std::cerr << "Application: Failed to create FullscreenManager.\n";
        return false;
    }
    std::cout << "FullscreenManager created.\n";

    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader)
    {
        std::cerr << "Application: Failed to create FontLoader.\n";
        return false;
    }
    std::cout << "FontLoader created.\n";

    m_fontData = std::make_unique<FontData>();
    if (!m_fontData)
    {
        std::cerr << "Application: Failed to create FontData unique_ptr.\n";
        return false;
    }
    std::cout << "FontData unique_ptr created.\n";

    // ChunkManager の初期化
    m_chunkManager = std::make_unique<ChunkManager>(CHUNK_GRID_SIZE, NOISE_SCALE, RENDER_DISTANCE_CHUNKS);
    if (!m_chunkManager)
    {
        std::cerr << "Application: Failed to create ChunkManager.\n";
        return false;
    }
    std::cout << "ChunkManager created.\n";


    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer)
    {
        std::cerr << "Application: Failed to create Renderer.\n";
        return false;
    }
    std::cout << "Renderer created.\n";

    m_inputManager->setWindow(m_windowContext->getWindow());
    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
                                                     { this->updateProjectionMatrix(w, h); });
    m_fullscreenManager->setMouseResetCallback([this]()
                                               {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        } });
    m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());

    std::cout << "Application::setupDependenciesAndLoadResources finished successfully.\n";
    return true;
}

void Application::processInput()
{
    static bool f11_last = false;
    if (glfwGetKey(m_windowContext->getWindow(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());
        f11_last = true;
    }
    else
        f11_last = false;

    if (m_inputManager)
        m_inputManager->processInput();

    auto key = [this](int k)
    { return glfwGetKey(m_windowContext->getWindow(), k) == GLFW_PRESS; };

    m_camera->processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A), key(GLFW_KEY_D), m_timer->getDeltaTime());
    m_camera->processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL), m_timer->getDeltaTime());
}

void Application::update()
{
    m_timer->tick();
    updateFpsAndPositionStrings();

    // プレイヤーの位置に基づいてチャンクマネージャーを更新
    if (m_chunkManager) {
        m_chunkManager->update(m_camera->getPosition());
    }
}

void Application::updateFpsAndPositionStrings()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    frameCount++;

    if (m_timer->getDeltaTime() > 0.0 && m_timer->getTotalTime() - lastFPSTime >= 1.0)
    {
        double fps = frameCount / (m_timer->getTotalTime() - lastFPSTime);
        m_fpsString = "FPS: " + std::to_string(static_cast<int>(fps));
        frameCount = 0;
        lastFPSTime = m_timer->getTotalTime();
    }

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

    m_renderer->beginFrame(glm::vec4(CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B, CLEAR_COLOR_A));

    glm::mat4 view = m_camera->getViewMatrix();

    // ChunkManager からすべてのチャンクのレンダリングデータを取得し、ループして描画
    if (m_chunkManager) {
        const auto& allRenderData = m_chunkManager->getAllRenderData();
        for (const auto& pair : allRenderData) {
            // pair.first はチャンクのワールド座標（glm::ivec3）
            glm::ivec3 chunkCoord = pair.first;
            // チャンクのワールド位置を計算し、model 行列を作成
            // CHUNK_GRID_SIZE はチャンクのサイズ（ボクセル数）
            // この model 行列が、チャンクを正しいワールド位置に移動させる役割を果たす
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
                chunkCoord.x * CHUNK_GRID_SIZE,
                chunkCoord.y * CHUNK_GRID_SIZE, // Y座標も考慮する場合
                chunkCoord.z * CHUNK_GRID_SIZE
            ));
            
            // Renderer::renderScene に model 行列を追加して呼び出す
            m_renderer->renderScene(m_projectionMatrix, view, pair.second, model);
        }
    }


    int w, h;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);

    m_renderer->endFrame();
}
void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 1000.0f); // 遠景クリップ面を大きくする
}