// src/application.cpp

#include <glad/glad.h>   // これを一番最初に置く

#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "noise/PerlinNoise2D.hpp"
#include <chrono>

const float Application::CLEAR_COLOR_R = 0.0f, Application::CLEAR_COLOR_G = 0.0f, Application::CLEAR_COLOR_B = 0.0f, Application::CLEAR_COLOR_A = 1.0f;

Application::Application(std::unique_ptr<WindowContext> windowContext, // WindowContextを受け取る
                         std::unique_ptr<Camera> camera,
                         std::unique_ptr<Timer> timer,
                         std::unique_ptr<InputManager> inputManager,
                         std::unique_ptr<FullscreenManager> fullscreenManager,
                         std::unique_ptr<Chunk> chunk,
                         std::unique_ptr<Renderer> renderer,
                         std::unique_ptr<FontData> fontData,
                         std::unique_ptr<FontLoader> fontLoader)
    : m_windowContext(std::move(windowContext)), // WindowContextをムーブ
      m_camera(std::move(camera)),
      m_timer(std::move(timer)),
      m_inputManager(std::move(inputManager)),
      m_fullscreenManager(std::move(fullscreenManager)),
      m_fontData(std::move(fontData)),
      m_chunk(std::move(chunk)),
      m_renderer(std::move(renderer)),
      m_fontLoader(std::move(fontLoader))
{
}

Application::~Application() {
    // GLFWの終了はWindowContextのデストラクタやmain関数で適切に処理される
}

bool Application::initialize()
{
    // ★GLFW関連の初期化はWindowContextで行われるので削除★
    // if (!initGLFW()) { /* error */ }
    // if (!createWindowAndContext()) { /* error */ }

    // WindowContextにコールバックを登録する
    setupCallbacks();

    // ここでFontDataをロードする（OpenGLコンテキストは既に作成されているはず）
    if (!m_fontLoader || !m_fontData) {
        std::cerr << "Error: FontLoader or FontData is null in Application::initialize.\n";
        return false;
    }
    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }
    std::cout << "FontData loaded by Application.\n";

    if (!initializeManagersAndLoadResources())
    {
        std::cerr << "Initialize managers failed.\n";
        return false;
    }

    if (m_renderer && !m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }
    std::cout << "Application initialized successfully.\n";
    return true;
}

void Application::setupCallbacks()
{
    // WindowContextのコールバックセッターを利用して登録
    m_windowContext->setFramebufferSizeCallback([this](int w, int h) {
        this->updateProjectionMatrix(w, h);
        glViewport(0, 0, w, h); // glViewportの更新はWindowContextのコールバックラッパーで行われるため、ここからは削除
    });

    m_windowContext->setCursorPosCallback([this](double xpos, double ypos) {
        if (this->m_inputManager) {
            this->m_inputManager->processMouseMovement(xpos, ypos);
        }
    });
}

// FontLoader関連の処理は既にApplication::initialize()に移動済み
bool Application::initializeManagersAndLoadResources()
{
    // nullチェックを追加 (DIで渡されなかった場合を考慮)
    if (!m_inputManager || !m_fullscreenManager || !m_chunk)
    {
        std::cerr << "One or more managers/resources not provided via DI.\n";
        return false;
    }


    unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    PerlinNoise2D perlin(seed);
    float scale = 0.05f;
    int m_size = m_chunk->getSize();
    for (int x = 0; x < m_size; ++x)
    {
        for (int y = 0; y < m_size; ++y)
        {
            for (int z = 0; z < m_size; ++z)
            {
                m_chunk->setVoxel(x, y, z, (perlin.noise(x * scale, z * scale) * 6) + 5 >= y);
            }
        }
    }




    // InputManager にウィンドウポインタを設定 (WindowContextから取得)
    m_inputManager->setWindow(m_windowContext->getWindow());

    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
                                                      { this->updateProjectionMatrix(w, h); });
    m_fullscreenManager->setMouseResetCallback([this]()
                                                {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        } });
    // FullscreenManagerのtoggleFullscreenにはGLFWwindow*が必要
    m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());

    return true;
}

void Application::run()
{
    while (!m_windowContext->shouldClose()) // WindowContext経由でクローズ状態を確認
    {
        processInput();
        update();
        render();
        m_windowContext->swapBuffers(); // WindowContext経由でバッファスワップ
        m_windowContext->pollEvents();  // WindowContext経由でイベントポーリング
    }
}

void Application::processInput()
{
    static bool f11_last = false;
    // WindowContextからウィンドウポインタを取得
    if (glfwGetKey(m_windowContext->getWindow(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow()); // WindowContextからウィンドウポインタを取得
        f11_last = true;
    }
    else
        f11_last = false;

    if (m_inputManager)
        m_inputManager->processInput();

    auto key = [this](int k)
    { return glfwGetKey(m_windowContext->getWindow(), k) == GLFW_PRESS; }; // WindowContextからウィンドウポインタを取得
    m_camera->processMovementVector(key(GLFW_KEY_W), key(GLFW_KEY_S), key(GLFW_KEY_A), key(GLFW_KEY_D), m_timer->getDeltaTime());
    m_camera->processVerticalMovement(key(GLFW_KEY_SPACE), key(GLFW_KEY_LEFT_CONTROL), m_timer->getDeltaTime());
}

// update(), updateFpsAndPositionStrings(), render() は WindowContext に直接依存しないので変更なし
void Application::update()
{
    m_timer->tick();
    updateFpsAndPositionStrings();
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
    m_renderer->renderScene(m_projectionMatrix, view, *m_chunk, m_cubeSpacing);
    int w, h;
    // WindowContextから現在のウィンドウサイズを取得
    glfwGetFramebufferSize(m_windowContext->getWindow(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);
    m_renderer->endFrame();
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 100.0f);
}