#include "game_application.hpp"
#include <utility>
#include <iostream>

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
      m_windowContext(nullptr)
{
    std::cout << "--- GameApplication constructor called. ---\n";
}

GameApplication::~GameApplication()
{
    std::cout << "--- GameApplication destructor called. ---\n";
    glfwTerminate();
}

bool GameApplication::setupDependencies()
{
    std::cout << "GameApplication::setupDependencies started.\n";

    m_windowContext = std::make_unique<WindowContext>("Hello OpenGL Cubes", SCR_WIDTH, SCR_HEIGHT);
    if (!m_windowContext || !m_windowContext->initialize())
    {
        std::cerr << "GameApplication: Failed to create or initialize WindowContext.\n";
        return false;
    }
    std::cout << "WindowContext created and initialized.\n";

    m_camera = std::make_unique<Camera>(glm::vec3(0.0f));
    if (!m_camera)
    {
        std::cerr << "GameApplication: Failed to create Camera.\n";
        return false;
    }
    std::cout << "Camera created.\n";

    m_timer = std::make_unique<Timer>();
    if (!m_timer)
    {
        std::cerr << "GameApplication: Failed to create Timer.\n";
        return false;
    }
    std::cout << "Timer created.\n";

    m_inputManager = std::make_unique<InputManager>(*m_camera);
    if (!m_inputManager)
    {
        std::cerr << "GameApplication: Failed to create InputManager.\n";
        return false;
    }
    std::cout << "InputManager created.\n";

    m_fullscreenManager = std::make_unique<FullscreenManager>();
    if (!m_fullscreenManager)
    {
        std::cerr << "GameApplication: Failed to create FullscreenManager.\n";
        return false;
    }
    std::cout << "FullscreenManager created.\n";

    m_fontLoader = std::make_unique<FontLoader>();
    if (!m_fontLoader)
    {
        std::cerr << "GameApplication: Failed to create FontLoader.\n";
        return false;
    }
    std::cout << "FontLoader created.\n";

    std::cout << "Attempting to create FontData unique_ptr.\n";
    m_fontData = std::make_unique<FontData>();
    if (!m_fontData)
    {
        std::cerr << "GameApplication: Failed to create FontData unique_ptr.\n";
        return false;
    }
    std::cout << "FontData unique_ptr created.\n";

    m_chunk = std::make_unique<Chunk>(CHUNK_GRID_SIZE);
    if (!m_chunk)
    {
        std::cerr << "GameApplication: Failed to create Chunk.\n";
        return false;
    }
    std::cout << "Chunk created.\n";

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer)
    {
        std::cerr << "GameApplication: Failed to create Renderer.\n";
        return false;
    }
    std::cout << "Renderer created.\n";

    m_app = std::make_unique<Application>(
        std::move(m_windowContext),
        std::move(m_camera),
        std::move(m_timer),
        std::move(m_inputManager),
        std::move(m_fullscreenManager),
        std::move(m_chunk),
        std::move(m_renderer),
        std::move(m_fontData),
        std::move(m_fontLoader));

    if (!m_app)
    {
        std::cerr << "GameApplication: Failed to create core Application instance.\n";
        return false;
    }
    std::cout << "Core Application instance created.\n";

    std::cout << "GameApplication::setupDependencies finished successfully.\n";
    return true;
}

int GameApplication::run()
{
    if (!setupDependencies())
    {
        std::cerr << "GameApplication: Failed to setup dependencies. Exiting.\n";
        return -1;
    }
    std::cout << "GameApplication: Dependencies setup completed. Proceeding to Application initialize.\n";

    if (!m_app->initialize())
    {
        std::cerr << "GameApplication: Core Application initialization failed. Exiting.\n";
        return -1;
    }
    std::cout << "GameApplication: Application initialized. Starting run loop.\n";

    m_app->run();

    std::cout << "GameApplication: Application run loop finished.\n";
    return 0;
}
