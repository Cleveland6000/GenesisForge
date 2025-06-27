// src/application.cpp

#include <glad/glad.h>

#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "noise/perlin_noise_2d.hpp"
#include <chrono>

#include "chunk_mesh_generator.hpp" 
#include "chunk_renderer.hpp"     

const float Application::CLEAR_COLOR_R = 0.0f, Application::CLEAR_COLOR_G = 0.0f, Application::CLEAR_COLOR_B = 0.0f, Application::CLEAR_COLOR_A = 1.0f;

Application::Application(std::unique_ptr<WindowContext> windowContext,
                         std::unique_ptr<Camera> camera,
                         std::unique_ptr<Timer> timer,
                         std::unique_ptr<InputManager> inputManager,
                         std::unique_ptr<FullscreenManager> fullscreenManager,
                         std::unique_ptr<Chunk> chunk,
                         std::unique_ptr<Renderer> renderer,
                         std::unique_ptr<FontData> fontData,
                         std::unique_ptr<FontLoader> fontLoader)
    : m_windowContext(std::move(windowContext)),
      m_camera(std::move(camera)),
      m_timer(std::move(timer)),
      m_inputManager(std::move(inputManager)),
      m_fullscreenManager(std::move(fullscreenManager)),
      m_fontData(std::move(fontData)),
      m_chunk(std::move(chunk)),
      m_renderer(std::move(renderer)),
      m_fontLoader(std::move(fontLoader)),
      m_testCubeRenderData() 
{
}

Application::~Application() {
    // GLFWの終了はWindowContextのデストラクタやmain関数で適切に処理される
}

bool Application::initialize()
{
    setupCallbacks();

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

    // !!!ここが変更点!!! ChunkMeshGenerator::generateCubeMesh() の代わりに
    // ChunkMeshGenerator::generateMesh() を呼び出す
    ChunkMeshData chunkMeshData = ChunkMeshGenerator::generateMesh(*m_chunk, m_cubeSpacing);
    m_testCubeRenderData = ChunkRenderer::createChunkRenderData(chunkMeshData); 

    int initialWidth, initialHeight;
    glfwGetFramebufferSize(m_windowContext->getWindow(), &initialWidth, &initialHeight);
    updateProjectionMatrix(initialWidth, initialHeight);


    std::cout << "Application initialized successfully.\n";
    return true;
}

void Application::setupCallbacks()
{
    m_windowContext->setFramebufferSizeCallback([this](int w, int h) {
        this->updateProjectionMatrix(w, h);
    });

    m_windowContext->setCursorPosCallback([this](double xpos, double ypos) {
        if (this->m_inputManager) {
            this->m_inputManager->processMouseMovement(xpos, ypos);
        }
    });
}

bool Application::initializeManagersAndLoadResources()
{
    if (!m_inputManager || !m_fullscreenManager || !m_chunk)
    {
        std::cerr << "One or more managers/resources not provided via DI.\n";
        return false;
    }

    // Perlin Noise によるチャンクのボクセル設定ロジックはそのまま
    unsigned int seed = static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count());
    PerlinNoise2D perlin(seed);
    float scale = 0.05f;
    int m_size = m_chunk->getSize(); // Chunkのサイズを取得
    for (int x = 0; x < m_size; ++x)
    {
        for (int y = 0; y < m_size; ++y)
        {
            for (int z = 0; z < m_size; ++z)
            {
                // ここでボクセルデータが設定される
                m_chunk->setVoxel(x, y, z, (perlin.noise(x * scale, z * scale) * 6) + 5 >= y);
            }
        }
    }

    m_inputManager->setWindow(m_windowContext->getWindow());

    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
                                                      { this->updateProjectionMatrix(w, h); });
    m_fullscreenManager->setMouseResetCallback([this]()
                                                {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        } });
    m_fullscreenManager->toggleFullscreen(m_windowContext->getWindow());

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
    
    // Chunk全体のレンダリングデータを渡す
    m_renderer->renderScene(m_projectionMatrix, view, m_testCubeRenderData); 

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
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 100.0f);
}