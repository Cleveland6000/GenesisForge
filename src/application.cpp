#include <glad/glad.h>
#include "application.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

const float Application::CLEAR_COLOR_R = 0.0f, Application::CLEAR_COLOR_G = 0.0f, Application::CLEAR_COLOR_B = 0.0f, Application::CLEAR_COLOR_A = 1.0f;

// コンストラクタ引数からFontLoaderを削除し、FontDataは参照で受け取る
Application::Application(std::unique_ptr<Camera> camera,
                         std::unique_ptr<Timer> timer,
                         std::unique_ptr<InputManager> inputManager,
                         std::unique_ptr<FullscreenManager> fullscreenManager,
                         std::unique_ptr<Chunk> chunk,
                         std::unique_ptr<Renderer> renderer,
                         std::unique_ptr<FontData> fontData,
                         std::unique_ptr<FontLoader> fontLoader) // FontLoaderを追加
    : m_window(nullptr, glfwDestroyWindow),
      m_camera(std::move(camera)),
      m_timer(std::move(timer)),
      m_inputManager(std::move(inputManager)),
      m_fullscreenManager(std::move(fullscreenManager)),
      m_fontData(std::move(fontData)), // FontDataの所有権を移動
      m_chunk(std::move(chunk)),
      m_renderer(std::move(renderer)),
      m_fontLoader(std::move(fontLoader)) // FontLoaderの所有権を移動
{
    std::cout << "--- Application constructor called. ---\n";
}

Application::~Application() { glfwTerminate(); }

bool Application::initialize()
{
    if (!initGLFW())
    { /* error */
    }
    if (!createWindowAndContext())
    { /* error */
    } // ここでOpenGLコンテキストが作成される
    setupCallbacks();

    // ★★★ ここでFontDataをロードする！ ★★★
    if (!m_fontLoader->loadSDFont("../assets/fonts/NotoSansJP-VariableFont_wght.json", "../assets/fonts/noto_sans_jp_atlas.png", *m_fontData))
    {
        std::cerr << "Application: Failed to load font data." << std::endl;
        return false;
    }
    std::cout << "FontData loaded by Application.\n";

    if (!initializeManagersAndLoadResources())
    { /* error */
    }

    if (m_renderer && !m_renderer->initialize(*m_fontData))
    {
        std::cerr << "Failed to initialize Renderer.\n";
        return false;
    }
    std::cout << "Application initialized successfully.\n";
    return true;
}

bool Application::initGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return true;
}

bool Application::createWindowAndContext()
{
    m_window.reset(glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello OpenGL Cubes", NULL, NULL));
    if (!m_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }
    GLFWmonitor *primary = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary);
    glfwSetWindowPos(m_window.get(), (mode->width - SCR_WIDTH) / 2, (mode->height - SCR_HEIGHT) / 2);
    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    return true;
}

void Application::setupCallbacks()
{
    auto win = m_window.get();
    glfwSetWindowUserPointer(win, this);
    glfwSetFramebufferSizeCallback(win, Application::staticFramebufferSizeCallback);
    glfwSetCursorPosCallback(win, Application::staticMouseCallback);
}

// FontLoader関連の処理を削除
bool Application::initializeManagersAndLoadResources()
{
    // nullチェックを追加 (DIで渡されなかった場合を考慮)
    if (!m_inputManager || !m_fullscreenManager || !m_chunk)
    {
        std::cerr << "One or more managers/resources not provided via DI.\n";
        return false;
    }

    // InputManager にウィンドウポインタを設定
    m_inputManager->setWindow(m_window.get());

    m_fullscreenManager->setWindowSizeChangeCallback([this](int w, int h)
                                                     { this->updateProjectionMatrix(w, h); });
    m_fullscreenManager->setMouseResetCallback([this]()
                                               {
        if (this->m_inputManager) {
            this->m_inputManager->resetMouseState();
        } });
    m_fullscreenManager->toggleFullscreen(m_window.get());

    // FontDataのロードはGameApplicationで行われるため、ここからは削除
    // ApplicationはFontDataの参照を持つのみ

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(m_window.get()))
    {
        processInput();
        update();
        render();
        glfwSwapBuffers(m_window.get());
        glfwPollEvents();
    }
}

void Application::processInput()
{
    static bool f11_last = false;
    if (glfwGetKey(m_window.get(), GLFW_KEY_F11) == GLFW_PRESS)
    {
        if (!f11_last)
            m_fullscreenManager->toggleFullscreen(m_window.get());
        f11_last = true;
    }
    else
        f11_last = false;

    if (m_inputManager)
        m_inputManager->processInput();

    auto key = [this](int k)
    { return glfwGetKey(m_window.get(), k) == GLFW_PRESS; };
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
    m_renderer->renderScene(m_projectionMatrix, view, *m_chunk, m_cubeSpacing);
    int w, h;
    glfwGetFramebufferSize(m_window.get(), &w, &h);
    m_renderer->renderOverlay(w, h, m_fpsString, m_positionString);
    m_renderer->endFrame();
}

void Application::staticFramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        app->updateProjectionMatrix(width, height);
    glViewport(0, 0, width, height);
}

void Application::updateProjectionMatrix(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    float aspect = (float)width / (float)height;
    m_projectionMatrix = glm::perspective(glm::radians(m_camera->Zoom), aspect, 0.1f, 100.0f);
}

void Application::staticMouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (auto app = static_cast<Application *>(glfwGetWindowUserPointer(window)); app && app->m_inputManager)
        app->m_inputManager->processMouseMovement(xpos, ypos);
}